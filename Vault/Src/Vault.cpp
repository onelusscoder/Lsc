#include <Lsc/Vault/Vault.h>
#include <Lsc/Vault/Data/Query.h>
#ifdef _WIN32
#include <shlwapi.h>
#else
    #include <unistd.h>
#endif




namespace Lsc::Vault
{

Vault::Vault::~Vault()
{
    Rem::Debug() << __PRETTY_FUNCTION__ << ": ";
    if(mDB)
        Close();
    else
        Rem::Debug() << __PRETTY_FUNCTION__ << ": Clear.";
    if(!mTables.empty())
        mTables.clear();
}



Vault::Vault::Vault(std::string DbName_):
mName(std::move(DbName_))
{

}

/*!
 * @brief   Opens the sqlite3 database file.
 *
 * @note Do not let sqlite3_open auto-create the file automatically.
 * @return sqlitedb::code::ok.
*/

Return Vault::Open()
{
    String str = mName;
    str << ".sqlite3";
#ifdef  _WIN32
    if (!PathFileExistsA(str().c_str()))
        return Rem::Error() << " sqlite3 open db error(" << mName <<  ") - no such database file.\n";
    
#else
    if(access(str().c_str(), F_OK|R_OK|W_OK) != 0) // Just check if the db file exists.
        throw Rem::Error() << "error openning database '" << str() << "' - database file not found.";
    
#endif
    int res = sqlite3_open(str().c_str(), &mDB);
    if (res != SQLITE_OK)
        throw Rem::Error()<< " sqlitedb open db error(" << res << ") - " << sqlite3_errmsg(mDB);

    std::cout << " database is open...\n";

    
    Expect<std::size_t> R = PullSchema();
    if(!R)
        throw R();
    
    Rem::Success() << __PRETTY_FUNCTION__ <<": " << *R << " Tables in DB " << mName;
    return Rem::Int::Accepted;
}


Return Vault::Close()
{
    if(!mDB)
        return Rem::Warning() << "Vault::Close() : " << Rem::Int::Rejected;
    sqlite3_close(mDB);
    mDB = nullptr;
    return Rem::Int::Accepted;
}


Return Vault::Create()
{
    String str = mName;
    str << ".sqlite3";
#ifdef  _WIN32
    if (PathFileExistsA(str().c_str()))
        return Rem::Error() << " sqlitedb create db error - Database file exists.";

#else
    int exists = access(str().c_str(), F_OK) == 0; // Just check if the db file exists.
    if (exists)
        throw Rem::Error() << "error creating database '" << mName << "' - database file already exists.";
#endif
    int res = sqlite3_open(str().c_str(), &mDB);
    if (res != SQLITE_OK)
        throw Rem::Error() << " sqlite3 create db error(" << res <<  ") - " <<  sqlite3_errmsg(mDB);
    
    return Rem::Int::Accepted;
}


Expect<std::size_t> Vault::PullSchema()
{
    const char *sql = "SELECT name FROM sqlite_master WHERE type='table' and name <> 'sqlite_sequence' order by name;";
    if(!mDB)
        return Rem::Error() << __PRETTY_FUNCTION__ << '(' << "'" << mName << "') - Database File not open.";
    
    auto Ln = [](void *this_, int argc, char **argv, char **azColName) -> int{
        auto* This = reinterpret_cast<Vault*>(this_);
        if(!This)
        {
            Rem::Error() << __PRETTY_FUNCTION__ << ": Cannot get the Vault instance from the sqlite3_exec.";
            return -1;
        }
        for(int i = 0; i < argc; i++)
            //if(std::string(argv[i]) != "sqlite_sequence")
                This->mTables.push_back({argv[i],This});
        
        return 0;
    };
    
    char* ErrMsg;
    
    int Ok = sqlite3_exec(mDB, sql, Ln, this, &ErrMsg);
    if(Ok != SQLITE_OK)
    {
        
        Expect<std::size_t> R = Rem::Error() << __PRETTY_FUNCTION__ << ": " << ErrMsg;
        sqlite3_free(ErrMsg);
        return R;
    }
    
    for(auto& Tbl : mTables)
    {
        Tbl.PullSchema();
        Rem::Debug() << __PRETTY_FUNCTION__ << ": " << Tbl.Serialize() << "...";
    }
    return mTables.size();
}


Expect<Table *> Vault::operator[](std::string Name_)
{
//    if(mTables.empty())
//        return Rem::Error() << __PRETTY_FUNCTION__ << ": No such table schema named '" <<
//            Name_ << "' in Vault, named '" << mName << "'";
    
    for(auto& T : mTables)
    {
        if(T.mName == Name_)
            return &T;
    }
    
    return Rem::Error() << __PRETTY_FUNCTION__ << ": No such table schema named '" <<
        Name_ << "' in Vault, named '" << mName << "'";
}


Return Vault::ExecuteQuery(Query &Q)
{
    if(!mDB)
        throw Rem::Exception() << __PRETTY_FUNCTION__ << ": null sqite3 handle: Is the DB file open ?";
    
    char * ErrStr;
    Rem::Debug() << __PRETTY_FUNCTION__ << ": SQL: [" << Q.SqlText() << "]:";
    int Ok  = sqlite3_exec(mDB,Q.SqlText().c_str(), nullptr,nullptr, &ErrStr);
    if(Ok)
    {
        std::string str = ErrStr;
        sqlite3_free(ErrStr);
        throw Rem::Exception() << __PRETTY_FUNCTION__ << ":" << str;
    }
    
    return Rem::Int::Ok;
}


Expect<Table *> Vault::NewTable(const std::string &Name_)
{
    mTables.push_back({Name_,this});
    return &mTables.back();
}

}
