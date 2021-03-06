#include <Lsc/Vault/Vault.h>
#include <Lsc/Vault/Model/Table.h>




namespace  Lsc::Vault
{




Table::Table(std::string Name_, Vault *Vault_) : mName(std::move(Name_)), mVault(Vault_)
{
    
}

Lsc::Expect<size_t> Table::PullSchema()
{
    String sql = "PRAGMA table_info('%s')";
    sql << mName;
    
    char *ErrMsg = nullptr;
    auto Lm = [](void *this_, int argc, char **argv, char **azColName)->int {
        auto *This = reinterpret_cast<Table *>(this_);
        Field::SchemaInfo Infos;
        for(int i = 0; i < argc; i++)
        {
            Rem::Debug() << "[" << azColName[i] << "]: " << argv[i];
            Infos.push_back({azColName[i], argv[i]});
        }
        This->mFields.push_back({This, Infos});
        Infos.clear();
        return SQLITE_OK;
    };
    
    if(SQLITE_OK != sqlite3_exec(mVault->Handle(), sql.c_str(), Lm, this, &ErrMsg))
    {
        Expect<std::size_t> R = Rem::Error() << "Table::PullSchema: sqlite3: " << sqlite3_errmsg(mVault->Handle());
        sqlite3_free(ErrMsg);
        return R;
    }
    
    return mFields.size();
}

sqlite3 *Table::DB()
{
    return mVault->Handle();
}

Expect<Field *> Table::operator[](const std::string &Name_)
{
    if(mFields.empty())
    {
        throw Rem::Exception() << __PRETTY_FUNCTION__ << ": No such Field named '" << Name_ << "' in Table '" << mName << "'";
    }
    
    for(auto& F : mFields)
    {
        if(F.Name() == Name_) return &F;
    }
    
    throw Rem::Exception() << __PRETTY_FUNCTION__ << ": No such Field named '" << Name_ << "' in Table '" << mName << "'";
}


Table::~Table()
{
    mFields.clear();
    Rem::Debug() << __PRETTY_FUNCTION__ << "["<< mName << "]: \\O/!";
}


Table &Table::operator<<(Field &&F_)
{
    mFields.emplace_back(F_);
    return *this;
}

String Table::Serialize()
{
    String Str = "CREATE TABLE %s(%s);";
    String StrFields;
    Rem::Debug() << __PRETTY_FUNCTION__ << ": " << mFields.size() << " Fields:";
    std::size_t fn=0l, fc=mFields.size();
    
    for(auto F : mFields)
    {
        StrFields += F.Serialize()();
        ++fn;
        if(fn < fc)
            StrFields += ", ";
    }
    Str << mName << StrFields();
    Rem::Debug() << __PRETTY_FUNCTION__ << ": " << Str();
    return Str;
}
}
