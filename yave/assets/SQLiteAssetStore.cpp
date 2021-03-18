/*******************************
Copyright (c) 2016-2021 Grégoire Angerand

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

#include <y/utils/log.h>

#ifndef YAVE_NO_SQLITE

#include <external/sqlite/sqlite3.h>

#include <thread>


// https://stackoverflow.com/questions/1711631/improve-insert-per-second-performance-of-sqlite

namespace yave {
static bool is_row(int res) {
    return res == SQLITE_ROW;
}

static bool is_done(int res) {
    return res == SQLITE_DONE;
}

static bool is_ok(int res) {
    return res == SQLITE_OK;
}

static int step_db(sqlite3_stmt* stmt) {
    y_profile();
    return sqlite3_step(stmt);
}

template<auto F = sqlite3_column_text>
static auto rows(sqlite3_stmt* stmt, int col = 0) {
    class RowIterator {
        public:
            RowIterator(sqlite3_stmt* stmt, int col) : _stmt(stmt), _col(col) {
                operator++(); // get first row
            }

            RowIterator& operator++() {
                if(_stmt && !is_row(step_db(_stmt))) {
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
    return core::Ok(core::String());
}

FileSystemModel::Result<bool> SQLiteAssetStore::SQLiteFileSystemModel::exists(std::string_view path) const {
    y_profile();

    const bool has_delim = !path.empty() && is_delimiter(path.back());

    sqlite3_stmt* stmt = nullptr;
    check(sqlite3_prepare_v2(_database, "SELECT 1 FROM Assets WHERE name = ? UNION SELECT 1 FROM Folders WHERE name = ?", -1, &stmt, nullptr));
    check(sqlite3_bind_text(stmt, 1, path.data(), int(path.size()), nullptr));
    check(sqlite3_bind_text(stmt, 2, path.data(), int(path.size() - has_delim), nullptr));
    y_defer(sqlite3_finalize(stmt));

    return core::Ok(is_row(step_db(stmt)));
}

FileSystemModel::Result<bool> SQLiteAssetStore::SQLiteFileSystemModel::is_directory(std::string_view path) const {
    y_profile();

    const bool has_delim = !path.empty() && is_delimiter(path.back());

    sqlite3_stmt* stmt = nullptr;
    check(sqlite3_prepare_v2(_database, "SELECT 1 FROM Folders WHERE name = ?", -1, &stmt, nullptr));
    check(sqlite3_bind_text(stmt, 1, path.data(), int(path.size() - has_delim), nullptr));
    y_defer(sqlite3_finalize(stmt));

    return core::Ok(is_row(step_db(stmt)));
}

FileSystemModel::Result<core::String> SQLiteAssetStore::SQLiteFileSystemModel::absolute(std::string_view path) const {
    return core::Ok(core::String(path));
}

FileSystemModel::Result<> SQLiteAssetStore::SQLiteFileSystemModel::for_each(std::string_view path, const for_each_f& func) const {
    y_profile();

    bool remove_delim = !path.empty();
    auto fid = folder_id(path);
    y_try(fid);

    {
        sqlite3_stmt* stmt = nullptr;
        check(sqlite3_prepare_v2(_database, "SELECT name FROM (SELECT name FROM Assets WHERE folderid = ? UNION SELECT name FROM Folders WHERE parentid = ?)", -1, &stmt, nullptr));
        check(sqlite3_bind_int64(stmt, 1, fid.unwrap()));
        check(sqlite3_bind_int64(stmt, 2, fid.unwrap()));
        y_defer(sqlite3_finalize(stmt));

        for(auto row : rows(stmt)) {
            std::string_view name = reinterpret_cast<const char*>(row);
            if(path.size() < name.size()) {
                name = name.substr(path.size() + remove_delim);
            }
            func(name);
        }
    }

    return core::Ok();
}

FileSystemModel::Result<> SQLiteAssetStore::SQLiteFileSystemModel::create_directory(std::string_view path) const {
    y_profile();

    auto parent = parent_path(path);
    y_try(parent);

    auto fid = folder_id(parent.unwrap());
    y_try(fid);


    {
        const bool has_delim = !path.empty() && is_delimiter(path.back());

        sqlite3_stmt* stmt = nullptr;
        check(sqlite3_prepare_v2(_database, "INSERT INTO Folders(name, folderid, parentid) VALUES(?, (SELECT MAX(folderid) FROM Folders) + 1, ?)", -1, &stmt, nullptr));
        check(sqlite3_bind_text(stmt, 1, path.data(), int(path.size() - has_delim), nullptr));
        check(sqlite3_bind_int64(stmt, 2, fid.unwrap()));
        y_defer(sqlite3_finalize(stmt));

        if(!is_done(step_db(stmt))) {
            return core::Err();
        }
    }
    log_msg(fmt("Folder created: %", path));

    return core::Ok();
}

FileSystemModel::Result<> SQLiteAssetStore::SQLiteFileSystemModel::remove(std::string_view path) const {
    y_profile();

    {
        const bool has_delim = !path.empty() && is_delimiter(path.back());
        const std::string_view no_delim(path.data(), path.size() - has_delim);

        core::String transaction;
        transaction.set_min_capacity(1024);
        fmt_into(transaction, R"#(
              BEGIN TRANSACTION;
                  DELETE FROM Folders WHERE name = "%";
                  DELETE FROM Assets WHERE name = "%";
                  DELETE FROM Folders WHERE name LIKE "%/%";
                  DELETE FROM Assets WHERE name LIKE "%/%";
              COMMIT;
            )#",
            no_delim,
            no_delim,
            no_delim, '%',
            no_delim, '%'
        );

        if(!is_ok(sqlite3_exec(_database, transaction.data(), nullptr, nullptr, nullptr))) {
            return core::Err();
        }
    }

    return core::Ok();
}

FileSystemModel::Result<> SQLiteAssetStore::SQLiteFileSystemModel::rename(std::string_view from, std::string_view to) const {
    y_profile();

    auto parent = parent_path(to);
    y_try(parent);

    auto fid = folder_id(parent.unwrap());
    y_try(fid);

    const bool to_has_delim = !to.empty() && is_delimiter(to.back());
    const bool from_has_delim = !from.empty() && is_delimiter(from.back());
    const bool is_folder = to_has_delim || from_has_delim || folder_id(from).is_ok();

    if(is_folder) {
        const core::String from_path_wildcard = core::String(from) + "/%";

        core::String transaction;
        transaction.set_min_capacity(1024);
        fmt_into(transaction, R"#(
              BEGIN TRANSACTION;
                  UPDATE Folders SET name = "%", parentid = % WHERE name = "%";
                  UPDATE Folders SET name = "%" + SUBSTR(name, %) WHERE name LIKE "%";
                  UPDATE Assets SET name = "%" + SUBSTR(name, %) WHERE name LIKE "%";
              COMMIT;
            )#",
            std::string_view(to.data(), to.size() - to_has_delim), fid.unwrap(), std::string_view(from.data(), from.size() - from_has_delim),
            to, from.size() + 1, from_path_wildcard,
            to, from.size() + 1, from_path_wildcard
        );

        if(!is_ok(sqlite3_exec(_database, transaction.data(), nullptr, nullptr, nullptr))) {
            return core::Err();
        }
    } else {
        sqlite3_stmt* stmt = nullptr;
        check(sqlite3_prepare_v2(_database, "UPDATE Assets SET name = ?, folderid = ? WHERE name = ?", -1, &stmt, nullptr));
        check(sqlite3_bind_text(stmt, 1, to.data(), int(to.size()), nullptr));
        check(sqlite3_bind_int64(stmt, 2, fid.unwrap()));
        check(sqlite3_bind_text(stmt, 3, from.data(), int(from.size()), nullptr));
        y_defer(sqlite3_finalize(stmt));

        if(!is_done(step_db(stmt))) {
            return core::Err();
        }
    }

    return core::Ok();
}

FileSystemModel::Result<core::Vector<core::String>> SQLiteAssetStore::SQLiteFileSystemModel::search(std::string_view pattern) const {
    y_profile();

    Y_TODO(This is still case sensitive, somehow)
    core::Vector<core::String> results;

    sqlite3_stmt* stmt = nullptr;
    check(sqlite3_prepare_v2(_database, "SELECT name FROM (SELECT name FROM Folders WHERE UPPER(name) LIKE UPPER(?) UNION SELECT name FROM Assets WHERE name LIKE ? COLLATE NOCASE)", -1, &stmt, nullptr));
    check(sqlite3_bind_text(stmt, 1, pattern.data(), int(pattern.size()), nullptr));
    check(sqlite3_bind_text(stmt, 2, pattern.data(), int(pattern.size()), nullptr));
    y_defer(sqlite3_finalize(stmt));

    for(auto row : rows(stmt)) {
        std::string_view name = reinterpret_cast<const char*>(row);
        results << name;
    }

    return core::Ok(std::move(results));
}

FileSystemModel::Result<i64> SQLiteAssetStore::SQLiteFileSystemModel::folder_id(std::string_view path) const {
    y_profile();

    const bool has_delim = !path.empty() && is_delimiter(path.back());

    if(path.empty() || (has_delim && path.size() == 1)) {
        return core::Ok(i64(0));
    }


    sqlite3_stmt* stmt = nullptr;
    check(sqlite3_prepare_v2(_database, "SELECT folderid FROM Folders WHERE name = ?", -1, &stmt, nullptr));
    check(sqlite3_bind_text(stmt, 1, path.data(), int(path.size() - has_delim), nullptr));
    y_defer(sqlite3_finalize(stmt));

    if(!is_row(step_db(stmt))) {
        return core::Err();
    }

    const i64 id = sqlite3_column_int64(stmt, 0);
    y_debug_assert(id != 0);
    return core::Ok(id);
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

    log_msg(fmt("Max BLOB length = % bytes", sqlite3_limit(_database, SQLITE_LIMIT_LENGTH, -1)));

    {
        sqlite3_stmt* stmt = nullptr;
        check(sqlite3_prepare_v2(_database, "SELECT name FROM sqlite_master WHERE type = 'table'", -1, &stmt, nullptr));
        y_defer(sqlite3_finalize(stmt));

        for(auto name : rows(stmt)) {
            log_msg(fmt("Found: TABLE %", reinterpret_cast<const char*>(name)), Log::Debug);
        }
    }
    {
        sqlite3_stmt* stmt = nullptr;
        check(sqlite3_prepare_v2(_database, "SELECT name FROM sqlite_master WHERE type = 'index'", -1, &stmt, nullptr));
        y_defer(sqlite3_finalize(stmt));

        for(auto name : rows(stmt)) {
            log_msg(fmt("Found: INDEX %", reinterpret_cast<const char*>(name)), Log::Debug);
        }
    }

    // Set pragmas
    {
        // Dangerous!!
        check(sqlite3_exec(_database, "PRAGMA synchronous = OFF", nullptr, nullptr, nullptr)); // unsafe if the OS crashes

        check(sqlite3_exec(_database, "PRAGMA page_size = 65536", nullptr, nullptr, nullptr));
        check(sqlite3_exec(_database, "PRAGMA temp_store = MEMORY", nullptr, nullptr, nullptr));
        check(sqlite3_exec(_database, "PRAGMA foreign_keys = ON", nullptr, nullptr, nullptr));
        check(sqlite3_exec(_database, "PRAGMA case_sensitive_like = ON", nullptr, nullptr, nullptr));
        check(sqlite3_exec(_database, "PRAGMA auto_vacuum = INCREMENTAL;", nullptr, nullptr, nullptr));
    }

    // Create tables & indexes
    {
        check(sqlite3_exec(_database, "CREATE TABLE IF NOT EXISTS Folders (name TEXT    PRIMARY KEY, folderid INTEGER UNIQUE, parentid INTEGER,"
                                            "FOREIGN KEY(parentid) REFERENCES Folders(folderid) ON DELETE CASCADE)", nullptr, nullptr, nullptr));
        check(sqlite3_exec(_database, "CREATE TABLE IF NOT EXISTS Assets  (uid  INTEGER PRIMARY KEY, name TEXT UNIQUE, folderid INTEGER, type INTEGER, data BLOB,"
                                            "FOREIGN KEY(folderid) REFERENCES Folders(folderid) ON DELETE CASCADE)", nullptr, nullptr, nullptr));

        /*check(sqlite3_exec(_database, "CREATE TABLE IF NOT EXISTS NextID (nextid INTEGER)", nullptr, nullptr, nullptr));
        check(sqlite3_exec(_database, "INSERT INTO NextID(nextid) SELECT (SELECT MAX(uid) + 1 FROM Assets) WHERE NOT EXISTS (SELECT 1 FROM NextID)", nullptr, nullptr, nullptr));*/

        check(sqlite3_exec(_database, "CREATE INDEX IF NOT EXISTS assetidindex ON Assets(uid)", nullptr, nullptr, nullptr));

    }

    // Create triggers
    {
        check(sqlite3_exec(_database, "DROP TRIGGER IF EXISTS renameassetstrigger", nullptr, nullptr, nullptr));
        check(sqlite3_exec(_database, "DROP TRIGGER IF EXISTS renamefolderstrigger", nullptr, nullptr, nullptr));

        check(sqlite3_exec(_database, "CREATE TRIGGER renameassetstrigger AFTER UPDATE ON Folders "
                                            "BEGIN UPDATE Assets SET name = NEW.name || SUBSTR(name, LENGTH(OLD.name) + 1) "
                                            "WHERE SUBSTR(name, 0, LENGTH(OLD.name) + 1) LIKE OLD.name; END", nullptr, nullptr, nullptr));
        check(sqlite3_exec(_database, "CREATE TRIGGER renamefolderstrigger AFTER UPDATE ON Folders "
                                            "BEGIN UPDATE Folders SET name = NEW.name || SUBSTR(name, LENGTH(OLD.name) + 1) "
                                            "WHERE SUBSTR(name, 0, LENGTH(OLD.name) + 1) LIKE OLD.name AND folderid <> NEW.folderid; END", nullptr, nullptr, nullptr));
    }


    // Create root folder
    check(sqlite3_exec(_database, "INSERT INTO Folders(name, folderid) SELECT '', 0 "
                                        "WHERE NOT EXISTS(SELECT 1 FROM Folders WHERE folderid = 0);", nullptr, nullptr, nullptr));

}

SQLiteAssetStore::~SQLiteAssetStore() {
    y_profile();

    /*u32 timeout = 8;
    while(sqlite3_next_stmt(_database, nullptr)) {
        log_msg("Waiting for DB to finish.");
        std::this_thread::sleep_for(std::chrono::milliseconds(timeout));
        timeout = std::min(timeout * 2, u32(1024));
    }*/

    while(sqlite3_stmt* stmt = sqlite3_next_stmt(_database, nullptr)) {
        log_msg(fmt("Database has pending statements (busy: %).", !!sqlite3_stmt_busy(stmt)), Log::Warning);
        sqlite3_finalize(stmt);
    }

    check(sqlite3_close(_database));
}

const FileSystemModel* SQLiteAssetStore::filesystem() const {
    return &_filesystem;
}


AssetStore::Result<AssetId> SQLiteAssetStore::import(io2::Reader& data, std::string_view dst_name, AssetType type) {
    y_profile();

    const core::String filename = _filesystem.filename(dst_name);
    const auto parent_path = _filesystem.parent_path(dst_name);

    if(filename.is_empty()) {
        return core::Err(ErrorType::InvalidName);
    }
    if(!parent_path) {
        return core::Err(ErrorType::FilesytemError);
    }

    if(const auto e = _filesystem.exists(dst_name); e.is_ok()) {
        if(e.unwrap()) {
            return core::Err(ErrorType::AlreadyExistingID);
        }
    } else {
        return core::Err(ErrorType::FilesytemError);
    }

    const auto folder_id = find_folder(parent_path.unwrap());
    if(!folder_id) {
        return core::Err(folder_id.error());
    }


    // this is not thread safe
    AssetId id = next_id();

    {
        sqlite3_stmt* stmt = nullptr;
        check(sqlite3_prepare_v2(_database, "INSERT INTO Assets(name, uid, folderid, type) VALUES(?, ?, ?, ?)", -1, &stmt, nullptr));
        check(sqlite3_bind_text(stmt, 1, dst_name.data(), int(dst_name.size()), nullptr));
        check(sqlite3_bind_int64(stmt, 2, id.id()));
        check(sqlite3_bind_int64(stmt, 3, folder_id.unwrap()));
        check(sqlite3_bind_int(stmt, 4, int(type)));
        y_defer(sqlite3_finalize(stmt));

        if(!is_done(step_db(stmt))) {
            remove(id).ignore();
            return core::Err(ErrorType::Unknown);
        }
    }

    if(const auto w = write(id, data); !w) {
        remove(id).ignore();
        return core::Err(w.error());
    }

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
        check(sqlite3_prepare_v2(_database, "UPDATE Assets SET data = ? WHERE uid = ?", -1, &stmt, nullptr));
        check(sqlite3_bind_blob(stmt, 1, buffer.data(), int(buffer.size()), nullptr));
        check(sqlite3_bind_int64(stmt, 2, id.id()));
        y_defer(sqlite3_finalize(stmt));

        if(!is_done(step_db(stmt))) {
            return core::Err(ErrorType::UnknownID);
        }
    }
    return core::Ok();
}

AssetStore::Result<AssetId> SQLiteAssetStore::id(std::string_view name) const {
    y_profile();

    sqlite3_stmt* stmt = nullptr;
    check(sqlite3_prepare_v2(_database, "SELECT uid FROM Assets WHERE name = ?", -1, &stmt, nullptr));
    check(sqlite3_bind_text(stmt, 1, name.data(), int(name.size()), nullptr));
    y_defer(sqlite3_finalize(stmt));

    if(!is_row(step_db(stmt))) {
        return core::Err(ErrorType::UnknownID);
    }

    return core::Ok(AssetId::from_id(sqlite3_column_int64(stmt, 0)));
}

AssetStore::Result<core::String> SQLiteAssetStore::name(AssetId id) const {
    y_profile();

    if(id == AssetId::invalid_id()) {
        return core::Err(ErrorType::UnknownID);
    }

    {
        auto lock = y_profile_unique_lock(_cache_lock);
        if(auto it = _name_cache.find(id); it != _name_cache.end()) {
            return core::Ok(it->second);
        }
    }

    sqlite3_stmt* stmt = nullptr;
    check(sqlite3_prepare_v2(_database, "SELECT name FROM Assets WHERE uid = ?", -1, &stmt, nullptr));
    check(sqlite3_bind_int64(stmt, 1, i64(id.id())));
    y_defer(sqlite3_finalize(stmt));

    if(!is_row(step_db(stmt))) {
        return core::Err(ErrorType::UnknownID);
    }

    const char* name_data = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    core::String name = name_data;

    {
        auto lock = y_profile_unique_lock(_cache_lock);
        _name_cache[id] = name;
    }

    return core::Ok(std::move(name));
}


class SQLiteBuffer final : public io2::Reader {
    public:
        SQLiteBuffer(sqlite3_stmt* stmt) : _stmt(stmt) {
            y_profile();
            _buffer = static_cast<const u8*>(sqlite3_column_blob(stmt, 0));
            _size = sqlite3_column_bytes(stmt, 0);
        }

        ~SQLiteBuffer() override {
            sqlite3_finalize(_stmt);
        }

        bool at_end() const override {
            return !remaining();
        }

        usize remaining() const override {
            return _size - _cursor;
        }

        void seek(usize byte) override {
            y_debug_assert(byte <= _size);
            _cursor = byte;
        }

        usize tell() const override {
            return _cursor;
        }

        io2::ReadResult read(void* data, usize bytes) override {
            if(remaining() < bytes) {
                return core::Err<usize>(0);
            }
            std::copy_n(&_buffer[_cursor], bytes, static_cast<u8*>(data));
            _cursor += bytes;
            return core::Ok();
        }

        io2::ReadUpToResult read_up_to(void* data, usize max_bytes) override {
            const usize max = std::min(max_bytes, remaining());
            y_debug_assert(_cursor < _size || !max);
            std::copy_n(&_buffer[_cursor], max, static_cast<u8*>(data));
            _cursor += max;
            y_debug_assert(_cursor < _size);
            return core::Ok(max);
        }

        io2::ReadUpToResult read_all(core::Vector<u8>& data) override {
            const usize r = _size - _cursor;
            data.push_back(_buffer + _cursor, _buffer + _size);
            _cursor = _size;
            return core::Ok(r);
        }

    private:
        sqlite3_stmt* _stmt = nullptr;
        const u8* _buffer = nullptr;
        usize _size = 0;
        usize _cursor = 0;
};


AssetStore::Result<io2::ReaderPtr> SQLiteAssetStore::data(AssetId id) const {
    y_profile();

    sqlite3_stmt* stmt = nullptr;
    check(sqlite3_prepare_v2(_database, "SELECT data FROM Assets WHERE uid = ?", -1, &stmt, nullptr));
    check(sqlite3_bind_int64(stmt, 1, i64(id.id())));

    if(!is_row(step_db(stmt))) {
        return core::Err(ErrorType::UnknownID);
    }

    auto buffer = std::make_unique<SQLiteBuffer>(stmt);
    return core::Ok(io2::ReaderPtr(std::move(buffer)));
}

AssetStore::Result<> SQLiteAssetStore::remove(AssetId id) {
    y_profile();

    clear_cache();

    sqlite3_stmt* stmt = nullptr;
    check(sqlite3_prepare_v2(_database, "DELETE FROM Assets WHERE uid = ?", -1, &stmt, nullptr));
    check(sqlite3_bind_int64(stmt, 1, i64(id.id())));
    y_defer(sqlite3_finalize(stmt));

    if(!is_done(step_db(stmt))) {
        return core::Err(ErrorType::UnknownID);
    }

    return core::Ok();
}

AssetStore::Result<> SQLiteAssetStore::rename(AssetId id, std::string_view new_name) {
    y_profile();

    clear_cache();

    auto na = name(id);
    y_try(na);

    return rename(na.unwrap(), new_name);
}

AssetStore::Result<> SQLiteAssetStore::remove(std::string_view name) {
    y_profile();

    clear_cache();

    if(!_filesystem.remove(name)) {
        return core::Err(ErrorType::FilesytemError);
    }

    return core::Ok();
}

AssetStore::Result<> SQLiteAssetStore::rename(std::string_view from, std::string_view to) {
    y_profile();

    clear_cache();

    if(!_filesystem.rename(from, to)) {
        return core::Err(ErrorType::FilesytemError);
    }

    return core::Ok();
}

AssetStore::Result<AssetType> SQLiteAssetStore::asset_type(AssetId id) const {
    y_profile();

    sqlite3_stmt* stmt = nullptr;
    check(sqlite3_prepare_v2(_database, "SELECT type FROM Assets WHERE uid = ?", -1, &stmt, nullptr));
    check(sqlite3_bind_int64(stmt, 1, i64(id.id())));
    y_defer(sqlite3_finalize(stmt));

    if(!is_row(step_db(stmt))) {
        return core::Err(ErrorType::UnknownID);
    }

    return core::Ok(AssetType(sqlite3_column_int(stmt, 0)));
}


AssetId SQLiteAssetStore::next_id() {
    y_profile();

    Y_TODO(This can recycle ids!)

    sqlite3_stmt* stmt = nullptr;
    check(sqlite3_prepare_v2(_database, "SELECT MAX(uid) FROM Assets", -1, &stmt, nullptr));
    y_defer(sqlite3_finalize(stmt));

    const auto to_id = [](i64 id) { return AssetIdFactory::create(u64(id)).create_id(); };
    if(!is_row(step_db(stmt))) {
        return to_id(std::numeric_limits<i64>::lowest());
    }
    const i64 max_id = sqlite3_column_int64(stmt, 0);
    return to_id(max_id + 1);
}

SQLiteAssetStore::Result<i64> SQLiteAssetStore::find_folder(std::string_view name, bool or_create) {
    if(const auto fid = _filesystem.folder_id(name)) {
        return core::Ok(fid.unwrap());
    }

    if(or_create) {
        if(_filesystem.create_directory(name)) {
            return find_folder(name, false);
        }
    }

    return core::Err(ErrorType::FilesytemError);
}

void SQLiteAssetStore::clear_cache() {
    y_profile();

    auto lock = y_profile_unique_lock(_cache_lock);
    _name_cache.make_empty();
}

}

#endif

