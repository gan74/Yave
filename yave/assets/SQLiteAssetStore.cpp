/*******************************
Copyright (c) 2016-2019 Gr�goire Angerand

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
**********************************/

#include "SQLiteAssetStore.h"

#ifndef YAVE_NO_SQLITE

#include <sqlite/sqlite3.h>


// https://stackoverflow.com/questions/1711631/improve-insert-per-second-performance-of-sqlite

namespace yave {
static bool is_row(int res) {
	return res == SQLITE_ROW;
}

static bool is_done(int res) {
	return res == SQLITE_DONE;
}

template<auto F = sqlite3_column_text>
static auto rows(sqlite3_stmt* stmt, int col = 0) {
	class RowIterator {
		public:
			RowIterator(sqlite3_stmt* stmt, int col) : _stmt(stmt), _col(col) {
				operator++(); // get first row
			}

			RowIterator& operator++() {
				if(_stmt && !is_row(sqlite3_step(_stmt))) {
					_stmt = nullptr;
				}
				return *this;
			}

			auto operator*() {
				return F(_stmt, _col);
			}

			bool operator!=(const RowIterator& other) const {
				return _stmt != other._stmt;
			}

		private:
			sqlite3_stmt* _stmt = nullptr;
			int _col = 0;
	};

	class Rows {
		public:
			Rows(sqlite3_stmt* stmt, int col) : _stmt(stmt), _col(col) {
			}

			RowIterator begin() {
				return RowIterator(_stmt, _col);
			}

			RowIterator end() {
				return RowIterator(nullptr, _col);
			}

			~Rows() {
				sqlite3_finalize(_stmt);
			}

		private:
			sqlite3_stmt* _stmt = nullptr;
			int _col = 0;
	};

	return Rows(stmt, col);
}


void SQLiteAssetStore::SQLiteFileSystemModel::check(int res) const {
	if(res != SQLITE_OK) {
		// We might leak memory here, but we don't care
		y_fatal(_database ? sqlite3_errmsg(_database) : "Unknown SQLite error.");
	}
}

bool SQLiteAssetStore::SQLiteFileSystemModel::is_delimiter(char c) const {
	return c == '\\' || c == '/';
}


core::String SQLiteAssetStore::SQLiteFileSystemModel::join(std::string_view path, std::string_view name) const {
	if(!path.size()) {
		return name;
	}
	char last = path.back();
	core::String result;
	result.set_min_capacity(path.size() + name.size() + 1);
	result += path;
	if(!is_delimiter(last)) {
		result.push_back('/');
	}
	result += name;
	return result;
}

core::String SQLiteAssetStore::SQLiteFileSystemModel::filename(std::string_view path) const {
	for(usize i = path.size(); i > 0; --i) {
		if(is_delimiter(path[i - 1])) {
			return path.substr(i);
		}
	}
	return path;
}

FileSystemModel::Result<core::String> SQLiteAssetStore::SQLiteFileSystemModel::current_path() const {
	return core::Ok(core::String());
}

FileSystemModel::Result<core::String> SQLiteAssetStore::SQLiteFileSystemModel::parent_path(std::string_view path) const {
	for(usize i = path.size(); i > 0; --i) {
		if(is_delimiter(path[i - 1])) {
			return core::Ok(core::String(path.substr(0, i - 1)));
		}
	}
	return core::Ok(core::String(path));
}

FileSystemModel::Result<bool> SQLiteAssetStore::SQLiteFileSystemModel::exists(std::string_view path) const {
	y_profile();
	core::String file = filename(path);
	if(file.is_empty()) {
		return is_directory(path);
	}
	sqlite3_stmt* stmt = nullptr;
	check(sqlite3_prepare_v2(_database, "SELECT 1 FROM Names, Folders WHERE Names.folderid = Folders.folderid AND Folders.name=? AND Names.name=?", -1, &stmt, nullptr));
	check(sqlite3_bind_text(stmt, 1, path.data(), path.size() - file.size(), nullptr));
	check(sqlite3_bind_text(stmt, 2, file.data(), file.size(), nullptr));
	y_defer(sqlite3_finalize(stmt));

	return core::Ok(is_row(sqlite3_step(stmt)));
}

FileSystemModel::Result<bool> SQLiteAssetStore::SQLiteFileSystemModel::is_directory(std::string_view path) const {
	y_profile();
	sqlite3_stmt* stmt = nullptr;
	check(sqlite3_prepare_v2(_database, "SELECT 1 FROM Folders WHERE name=?", -1, &stmt, nullptr));
	check(sqlite3_bind_text(stmt, 1, path.data(), path.size(), nullptr));
	y_defer(sqlite3_finalize(stmt));

	return core::Ok(is_row(sqlite3_step(stmt)));
}

FileSystemModel::Result<core::String> SQLiteAssetStore::SQLiteFileSystemModel::absolute(std::string_view path) const {
	return core::Ok(core::String(path));
}

FileSystemModel::Result<> SQLiteAssetStore::SQLiteFileSystemModel::for_each(std::string_view path, const for_each_f& func) const {
	return core::Err();
}

FileSystemModel::Result<> SQLiteAssetStore::SQLiteFileSystemModel::create_directory(std::string_view path) const {
	y_profile();
	sqlite3_stmt* stmt = nullptr;
	check(sqlite3_prepare_v2(_database, "INSERT INTO Folders(name) VALUES(?)", -1, &stmt, nullptr));
	check(sqlite3_bind_text(stmt, 1, path.data(), path.size(), nullptr));
	y_defer(sqlite3_finalize(stmt));

	if(is_done(sqlite3_step(stmt))) {
		return core::Ok();
	}
	return core::Err();
}

FileSystemModel::Result<> SQLiteAssetStore::SQLiteFileSystemModel::remove(std::string_view path) const {
	y_profile();
	sqlite3_stmt* stmt = nullptr;
	check(sqlite3_prepare_v2(_database, "DELETE FROM Folders WHERE name = ?", -1, &stmt, nullptr));
	check(sqlite3_bind_text(stmt, 1, path.data(), path.size(), nullptr));
	y_defer(sqlite3_finalize(stmt));

	if(is_done(sqlite3_step(stmt))) {
		return core::Ok();
	}
	return core::Err();
}

FileSystemModel::Result<> SQLiteAssetStore::SQLiteFileSystemModel::rename(std::string_view from, std::string_view to) const {
	y_profile();
	sqlite3_stmt* stmt = nullptr;
	check(sqlite3_prepare_v2(_database, "UPDATE Folders SET name = ? WHERE name = ?", -1, &stmt, nullptr));
	check(sqlite3_bind_text(stmt, 1, to.data(), to.size(), nullptr));
	check(sqlite3_bind_text(stmt, 2, from.data(), from.size(), nullptr));
	y_defer(sqlite3_finalize(stmt));

	if(is_done(sqlite3_step(stmt))) {
		return core::Ok();
	}
	return core::Err();
}



void SQLiteAssetStore::check(int res) const {
	if(res != SQLITE_OK) {
		// We might leak memory here, but we don't care
		y_fatal(_database ? sqlite3_errmsg(_database) : "Unknown SQLite error.");
	}
}

SQLiteAssetStore::SQLiteAssetStore(const core::String& path) {
	y_profile();
	check(sqlite3_open(path.data(), &_database));
	_filesystem._database = _database;

	{
		sqlite3_stmt* stmt = nullptr;
		check(sqlite3_prepare_v2(_database, "SELECT name FROM sqlite_master WHERE type = 'table'", -1, &stmt, nullptr));
		for(auto name : rows(stmt)) {
			log_msg(fmt("Found: TABLE %", name), Log::Debug);
		}
	}

	check(sqlite3_exec(_database, "CREATE TABLE IF NOT EXISTS Names (name TEXT PRIMARY KEY, uid INT, folderid INT);", nullptr, nullptr, nullptr));
	check(sqlite3_exec(_database, "CREATE TABLE IF NOT EXISTS Data (uid INT PRIMARY KEY, data BLOB);", nullptr, nullptr, nullptr));
	check(sqlite3_exec(_database, "CREATE TABLE IF NOT EXISTS Folders (name TEXT PRIMARY KEY, folderid INT);", nullptr, nullptr, nullptr));
	check(sqlite3_exec(_database, "PRAGMA cache_size = 1048576", nullptr, nullptr, nullptr));
	check(sqlite3_exec(_database, "PRAGMA page_size = 65536", nullptr, nullptr, nullptr));
	check(sqlite3_exec(_database, "PRAGMA temp_store = MEMORY", nullptr, nullptr, nullptr));

	// dangerous!!
	check(sqlite3_exec(_database, "PRAGMA synchronous = OFF", nullptr, nullptr, nullptr)); // unsafe if the OS crashes
}

SQLiteAssetStore::~SQLiteAssetStore() {
	check(sqlite3_close(_database));
}

const FileSystemModel* SQLiteAssetStore::filesystem() const {
	return &_filesystem;
}


AssetStore::Result<AssetId> SQLiteAssetStore::import(io2::Reader& data, std::string_view dst_name) {
	return core::Err(ErrorType::Unknown);
}

AssetStore::Result<> SQLiteAssetStore::write(AssetId id, io2::Reader& data) {
	return core::Err(ErrorType::Unknown);
}

AssetStore::Result<AssetId> SQLiteAssetStore::id(std::string_view name) const {
	return core::Err(ErrorType::Unknown);
}

AssetStore::Result<core::String> SQLiteAssetStore::name(AssetId id) const {
	return core::Err(ErrorType::Unknown);
}

AssetStore::Result<io2::ReaderPtr> SQLiteAssetStore::data(AssetId id) const {
	return core::Err(ErrorType::Unknown);
}

AssetStore::Result<> SQLiteAssetStore::remove(AssetId id) {
	return core::Err(ErrorType::Unknown);
}

AssetStore::Result<> SQLiteAssetStore::rename(AssetId id, std::string_view new_name) {
	return core::Err(ErrorType::Unknown);
}

AssetStore::Result<> SQLiteAssetStore::remove(std::string_view name) {
	return core::Err(ErrorType::Unknown);
}

AssetStore::Result<> SQLiteAssetStore::rename(std::string_view from, std::string_view to) {
	return core::Err(ErrorType::Unknown);
}


AssetId SQLiteAssetStore::next_id() {
	sqlite3_stmt* stmt = nullptr;
	check(sqlite3_prepare_v2(_database, "SELECT uid FROM Names ORDER BY uid LIMIT 1", -1, &stmt, nullptr));
	y_defer(sqlite3_finalize(stmt));

	auto to_id = [](i64 id) { return AssetIdFactory::create(u64(id)).create_id(); };
	if(!is_row(sqlite3_step(stmt))) {
		return to_id(std::numeric_limits<i64>::lowest());
	}
	i64 max_id = sqlite3_column_int64(stmt, 0);
	return to_id(max_id + 1);
}

i64 SQLiteAssetStore::next_folder_id() {
	sqlite3_stmt* stmt = nullptr;
	check(sqlite3_prepare_v2(_database, "SELECT folderid FROM Folders ORDER BY folderid LIMIT 1", -1, &stmt, nullptr));
	y_defer(sqlite3_finalize(stmt));

	if(!is_row(sqlite3_step(stmt))) {
		return std::numeric_limits<i64>::lowest();
	}
	i64 max_id = sqlite3_column_int64(stmt, 0);
	return max_id + 1;
}

}

#endif
