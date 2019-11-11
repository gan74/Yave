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

	bool is_folder = is_delimiter(path.back());
	sqlite3_stmt* stmt = nullptr;
	check(sqlite3_prepare_v2(_database, "SELECT 1 FROM Names WHERE name = ? UNION SELECT 1 FROM Folders WHERE name = ?", -1, &stmt, nullptr));
	check(sqlite3_bind_text(stmt, 1, path.data(), path.size(), nullptr));
	check(sqlite3_bind_text(stmt, 2, path.data(), path.size() - is_folder, nullptr));
	y_defer(sqlite3_finalize(stmt));

	return core::Ok(is_row(sqlite3_step(stmt)));
}

FileSystemModel::Result<bool> SQLiteAssetStore::SQLiteFileSystemModel::is_directory(std::string_view path) const {
	y_profile();

	sqlite3_stmt* stmt = nullptr;
	check(sqlite3_prepare_v2(_database, "SELECT 1 FROM Folders WHERE name = ?", -1, &stmt, nullptr));
	check(sqlite3_bind_text(stmt, 1, path.data(), path.size(), nullptr));
	y_defer(sqlite3_finalize(stmt));

	return core::Ok(is_row(sqlite3_step(stmt)));
}

FileSystemModel::Result<core::String> SQLiteAssetStore::SQLiteFileSystemModel::absolute(std::string_view path) const {
	return core::Ok(core::String(path));
}

FileSystemModel::Result<> SQLiteAssetStore::SQLiteFileSystemModel::for_each(std::string_view path, const for_each_f& func) const {
	y_profile();

	auto fid = folder_id(path);
	y_try(fid);

	{
		sqlite3_stmt* stmt = nullptr;
		check(sqlite3_prepare_v2(_database, "SELECT name FROM Names WHERE folderid = ?", -1, &stmt, nullptr));
		check(sqlite3_bind_int64(stmt, 1, fid.unwrap()));
		y_defer(sqlite3_finalize(stmt));

		for(auto row : rows(stmt, 0)) {
			const char* name_data = reinterpret_cast<const char*>(row);
			func(name_data);
		}
	}
	return core::Ok();
}

FileSystemModel::Result<> SQLiteAssetStore::SQLiteFileSystemModel::create_directory(std::string_view path) const {
	y_profile();

	{
		sqlite3_stmt* stmt = nullptr;
		check(sqlite3_prepare_v2(_database, "INSERT INTO Folders(name, folderid) VALUES(?, ?)", -1, &stmt, nullptr));
		check(sqlite3_bind_text(stmt, 1, path.data(), path.size(), nullptr));
		check(sqlite3_bind_int64(stmt, 2, next_folder_id()));
		y_defer(sqlite3_finalize(stmt));

		if(!is_done(sqlite3_step(stmt))) {
			return core::Err();
		}
	}
	return core::Ok();
}

FileSystemModel::Result<> SQLiteAssetStore::SQLiteFileSystemModel::remove(std::string_view path) const {
	y_profile();

	{
		sqlite3_stmt* stmt = nullptr;
		check(sqlite3_prepare_v2(_database, "DELETE FROM Folders WHERE name = ?", -1, &stmt, nullptr));
		check(sqlite3_bind_text(stmt, 1, path.data(), path.size(), nullptr));
		y_defer(sqlite3_finalize(stmt));

		if(!is_done(sqlite3_step(stmt))) {
			return core::Err();
		}
	}
	{
		sqlite3_stmt* stmt = nullptr;
		check(sqlite3_prepare_v2(_database, "DELETE FROM Names WHERE name = ?", -1, &stmt, nullptr));
		check(sqlite3_bind_text(stmt, 1, path.data(), path.size(), nullptr));
		y_defer(sqlite3_finalize(stmt));

		if(!is_done(sqlite3_step(stmt))) {
			return core::Err();
		}
	}
	return core::Ok();
}

FileSystemModel::Result<> SQLiteAssetStore::SQLiteFileSystemModel::rename(std::string_view from, std::string_view to) const {
	y_profile();

	{
		sqlite3_stmt* stmt = nullptr;
		check(sqlite3_prepare_v2(_database, "UPDATE Folders SET name = ? WHERE name = ?", -1, &stmt, nullptr));
		check(sqlite3_bind_text(stmt, 1, to.data(), to.size(), nullptr));
		check(sqlite3_bind_text(stmt, 2, from.data(), from.size(), nullptr));
		y_defer(sqlite3_finalize(stmt));

		if(!is_done(sqlite3_step(stmt))) {
			return core::Err();
		}
	}
	{
		sqlite3_stmt* stmt = nullptr;
		check(sqlite3_prepare_v2(_database, "UPDATE Names SET name = ? WHERE name = ?", -1, &stmt, nullptr));
		check(sqlite3_bind_text(stmt, 1, to.data(), to.size(), nullptr));
		check(sqlite3_bind_text(stmt, 2, from.data(), from.size(), nullptr));
		y_defer(sqlite3_finalize(stmt));

		if(!is_done(sqlite3_step(stmt))) {
			return core::Err();
		}
	}

	return core::Ok();
}

FileSystemModel::Result<i64> SQLiteAssetStore::SQLiteFileSystemModel::folder_id(std::string_view path) const {
	y_profile();

	sqlite3_stmt* stmt = nullptr;
	check(sqlite3_prepare_v2(_database, "SELECT folderid FROM Folders WHERE name = ?", -1, &stmt, nullptr));
	check(sqlite3_bind_text(stmt, 1, path.data(), path.size(), nullptr));
	y_defer(sqlite3_finalize(stmt));

	if(!is_row(sqlite3_step(stmt))) {
		return core::Err();
	}

	return core::Ok(sqlite3_column_int64(stmt, 0));
}

i64 SQLiteAssetStore::SQLiteFileSystemModel::next_folder_id() const {
	y_profile();

	sqlite3_stmt* stmt = nullptr;
	check(sqlite3_prepare_v2(_database, "SELECT folderid FROM Folders ORDER BY folderid LIMIT 1", -1, &stmt, nullptr));
	y_defer(sqlite3_finalize(stmt));

	if(!is_row(sqlite3_step(stmt))) {
		return 1;
	}
	i64 max_id = sqlite3_column_int64(stmt, 0);
	return std::max(max_id, i64(0)) + 1;
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
		y_defer(sqlite3_finalize(stmt));

		for(auto name : rows(stmt)) {
			log_msg(fmt("Found: TABLE %", name), Log::Debug);
		}
	}

	log_msg(fmt("Max BLOB length = % bytes", sqlite3_limit(_database, SQLITE_LIMIT_LENGTH, -1)));

	check(sqlite3_exec(_database, "PRAGMA cache_size = 1048576", nullptr, nullptr, nullptr));
	check(sqlite3_exec(_database, "PRAGMA page_size = 65536", nullptr, nullptr, nullptr));
	check(sqlite3_exec(_database, "PRAGMA temp_store = MEMORY", nullptr, nullptr, nullptr));

	check(sqlite3_exec(_database, "CREATE TABLE IF NOT EXISTS Data    (uid  INT  PRIMARY KEY, data BLOB);", nullptr, nullptr, nullptr));
	check(sqlite3_exec(_database, "CREATE TABLE IF NOT EXISTS Folders (name TEXT PRIMARY KEY, folderid INTEGER UNIQUE);", nullptr, nullptr, nullptr));
	check(sqlite3_exec(_database, "CREATE TABLE IF NOT EXISTS Names   (name TEXT PRIMARY KEY, uid INT, folderid INT,"
										"FOREIGN KEY(uid) REFERENCES Data(uid) ON DELETE CASCADE,"
										"FOREIGN KEY(folderid) REFERENCES Folders(folderid) ON DELETE CASCADE);", nullptr, nullptr, nullptr));

	check(sqlite3_exec(_database, "CREATE UNIQUE INDEX uidindex ON Data(uid)", nullptr, nullptr, nullptr));
	check(sqlite3_exec(_database, "CREATE UNIQUE INDEX nameindex ON Names(name)", nullptr, nullptr, nullptr));

	// dangerous!!
	check(sqlite3_exec(_database, "PRAGMA synchronous = OFF", nullptr, nullptr, nullptr)); // unsafe if the OS crashes
}

SQLiteAssetStore::~SQLiteAssetStore() {
	y_profile();

	check(sqlite3_close(_database));
}

const FileSystemModel* SQLiteAssetStore::filesystem() const {
	return &_filesystem;
}


AssetStore::Result<AssetId> SQLiteAssetStore::import(io2::Reader& data, std::string_view dst_name) {
	y_profile();

	core::String filename = _filesystem.filename(dst_name);
	auto parent_path = _filesystem.parent_path(dst_name);

	if(filename.is_empty()) {
		return core::Err(ErrorType::InvalidName);
	}
	if(!parent_path) {
		return core::Err(ErrorType::FilesytemError);
	}

	if(auto e = _filesystem.exists(dst_name); e.is_ok()) {
		if(e.unwrap()) {
			return core::Err(ErrorType::AlreadyExistingID);
		}
	} else {
		return core::Err(ErrorType::FilesytemError);
	}

	auto folder_id = find_folder(parent_path.unwrap());
	if(!folder_id) {
		return core::Err(folder_id.error());
	}

	AssetId id = next_id();

	{
		sqlite3_stmt* stmt = nullptr;
		check(sqlite3_prepare_v2(_database, "INSERT INTO Names(name, uid, folderid) VALUES(?, ?, ?)", -1, &stmt, nullptr));
		check(sqlite3_bind_text(stmt, 1, dst_name.data(), dst_name.size(), nullptr));
		check(sqlite3_bind_int64(stmt, 2, id.id()));
		check(sqlite3_bind_int64(stmt, 3, folder_id.unwrap()));
		y_defer(sqlite3_finalize(stmt));

		if(!is_done(sqlite3_step(stmt))) {
			return core::Err(ErrorType::Unknown);
		}
	}

	y_try(write(id, data));

	return core::Ok(id);
}

AssetStore::Result<> SQLiteAssetStore::write(AssetId id, io2::Reader& data) {
	y_profile();

	core::Vector<u8> buffer;
	if(!data.read_all(buffer) || buffer.size() > usize(std::numeric_limits<int>::max())) {
		return core::Err(ErrorType::FilesytemError);
	}

	{
		sqlite3_stmt* stmt = nullptr;
		check(sqlite3_prepare_v2(_database, "INSERT INTO Data(uid, data) VALUES(?, ?)", -1, &stmt, nullptr));
		check(sqlite3_bind_int64(stmt, 1, id.id()));
		check(sqlite3_bind_blob(stmt, 2, buffer.data(), buffer.size(), nullptr));
		y_defer(sqlite3_finalize(stmt));

		if(!is_done(sqlite3_step(stmt))) {
			remove(id).ignore();
			return core::Err(ErrorType::Unknown);
		}
	}
	return core::Ok();
}

AssetStore::Result<AssetId> SQLiteAssetStore::id(std::string_view name) const {
	y_profile();

	sqlite3_stmt* stmt = nullptr;
	check(sqlite3_prepare_v2(_database, "SELECT uid FROM Names WHERE name = ?", -1, &stmt, nullptr));
	check(sqlite3_bind_text(stmt, 1, name.data(), name.size(), nullptr));
	y_defer(sqlite3_finalize(stmt));

	if(!is_row(sqlite3_step(stmt))) {
		return core::Err(ErrorType::UnknownID);
	}

	return core::Ok(AssetId::from_id(sqlite3_column_int64(stmt, 0)));
}

AssetStore::Result<core::String> SQLiteAssetStore::name(AssetId id) const {
	y_profile();

	sqlite3_stmt* stmt = nullptr;
	check(sqlite3_prepare_v2(_database, "SELECT name FROM Names WHERE uid = ?", -1, &stmt, nullptr));
	check(sqlite3_bind_int64(stmt, 1, i64(id.id())));
	y_defer(sqlite3_finalize(stmt));

	if(!is_row(sqlite3_step(stmt))) {
		return core::Err(ErrorType::UnknownID);
	}

	const char* name_data = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
	return core::Ok(core::String(name_data));
}

AssetStore::Result<io2::ReaderPtr> SQLiteAssetStore::data(AssetId id) const {
	y_profile();

	sqlite3_stmt* stmt = nullptr;
	check(sqlite3_prepare_v2(_database, "SELECT data FROM Data WHERE uid = ?", -1, &stmt, nullptr));
	check(sqlite3_bind_int64(stmt, 1, i64(id.id())));
	y_defer(sqlite3_finalize(stmt));

	if(!is_row(sqlite3_step(stmt))) {
		return core::Err(ErrorType::UnknownID);
	}

	int size = sqlite3_column_bytes(stmt, 0);
	const void* data = sqlite3_column_blob(stmt, 0);

	auto buffer = std::make_unique<io2::Buffer>(size);
	if(!buffer->write(static_cast<const u8*>(data), size)) {
		return core::Err(ErrorType::FilesytemError);
	}

	buffer->reset();
	return core::Ok(io2::ReaderPtr(std::move(buffer)));
}

AssetStore::Result<> SQLiteAssetStore::remove(AssetId id) {
	y_profile();

	return core::Err(ErrorType::UnsupportedOperation);
}

AssetStore::Result<> SQLiteAssetStore::rename(AssetId id, std::string_view new_name) {
	y_profile();

	return core::Err(ErrorType::UnsupportedOperation);
}

AssetStore::Result<> SQLiteAssetStore::remove(std::string_view name) {
	y_profile();

	return core::Err(ErrorType::UnsupportedOperation);
}

AssetStore::Result<> SQLiteAssetStore::rename(std::string_view from, std::string_view to) {
	y_profile();

	return core::Err(ErrorType::UnsupportedOperation);
}

AssetId SQLiteAssetStore::next_id() {
	y_profile();

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

SQLiteAssetStore::Result<i64> SQLiteAssetStore::find_folder(std::string_view name, bool or_create) {
	if(name.empty()) {
		return core::Ok(i64(0));
	}

	if(auto fid = _filesystem.folder_id(name)) {
		return core::Ok(fid.unwrap());
	}

	if(or_create) {
		if(_filesystem.create_directory(name)) {
			return find_folder(name, false);
		}
	}

	return core::Err(ErrorType::FilesytemError);
}

}

#endif
