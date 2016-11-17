#include <iostream>
#include <fstream>

#include "config_generated.h"
#include "character_generated.h"
#include "jonoondb/jonoondb_api/database.h"

using namespace std;
using namespace flatbuffers;
using namespace jonoondb_api;
using namespace jonoondb_tutorial;

string ReadFile(const string& path, bool isBinary = true) {
  ifstream ifs(path, isBinary ? ios::binary : ios::in);
  if (!ifs.is_open()) {
    ostringstream ss;
    ss << "Failed to open file at path " << path;
    throw runtime_error(ss.str());
  }

  string fileContents;
  if (isBinary) {
    ifs.seekg(0, ios::end);
    fileContents.resize(static_cast<size_t>(ifs.tellg()));
    ifs.seekg(0, ios::beg);
    ifs.read(const_cast<char*>(fileContents.data()), fileContents.size());
  } else {
    ostringstream oss;
    oss << ifs.rdbuf();
    fileContents = oss.str();
  }

  if (ifs.bad()) {
    ostringstream ss;
    ss << "Failed to read the file at path " << path;
    throw runtime_error(ss.str());
  }

  return fileContents;
}

void TutorialOpenDatabase() {
  {
    // Open a database with default options
    Database db("/path/to/db",           // path where db files will be created
                "game_of_thrones"  // database name              
    );
  }

  {
    Options opt;
    opt.SetCreateDBIfMissing(false);
    Database db("/path/to/db",           // path where db files will be created
                "game_of_thrones", // database name 
                opt                // options             
    );
  }
}

void CleanupOldFiles() {
  remove("game_of_thrones.dat");
  remove("game_of_thrones_character.0");
}

int main(int argc, char** argv) {
  CleanupOldFiles();

  try {
    // Open a database with default options
    Database db(DB_PATH,           // path where db files will be created
                "game_of_thrones"  // database name              
    );

    // Read the binary flatbuffer schema from file.
    // This defines the collection schema.
    string schemaFolder = SCHEMA_PATH;
    auto schema = ReadFile(schemaFolder + "/character.bfbs");

    vector<IndexInfo> indexes;
    db.CreateCollection("character",                  // collection name   
                        SchemaType::FLAT_BUFFERS,     // collection schema type
                        schema,                       // collection schema
                        indexes                       // indexes to create
    );

    // lets construct and insert a character
    FlatBufferBuilder fbb;
    auto actor = CreateActor(fbb, fbb.CreateString("Peter Dinklage"),
                             fbb.CreateString("Morristown"),
                             fbb.CreateString("1969-06-11"));
    auto obj = CreateCharacter(fbb, fbb.CreateString("Tyrion Lannister"),
                               fbb.CreateString("Lannister"),
                               actor,
                               39, fbb.CreateString("Winter is Coming"));
    fbb.Finish(obj);

    Buffer tyrion(reinterpret_cast<char*>(fbb.GetBufferPointer()), // Buffer pointer
                  fbb.GetSize(), // Buffer size
                  fbb.GetSize(), // Buffer capacity
                  nullptr);      // Deleter func ptr, nullptr means don't delete memory

    db.Insert("character",   // collection name in which to insert
              tyrion         // data that is to be inserted
    );

    // lets construct and insert multiple play characters
    std::vector<Buffer> characters; // vector to hold all documents to be inserted
    fbb.Clear(); // This is necessary if we want to reuse flatbufferbuilder
    actor = CreateActor(fbb, fbb.CreateString("Kit Harington"),
                        fbb.CreateString("London"),
                        fbb.CreateString("1986-12-26"));
    obj = CreateCharacter(fbb, fbb.CreateString("Jon Snow"),
                          fbb.CreateString("Stark"),
                          actor,
                          21, fbb.CreateString("Winter is Coming"));
    fbb.Finish(obj);

    characters.push_back(
        Buffer(reinterpret_cast<char*>(fbb.GetBufferPointer()), // Buffer pointer
               fbb.GetSize()  // Buffer size
        )
    );

    fbb.Clear();
    actor = CreateActor(fbb, fbb.CreateString("Aidan Gillen"),
                        fbb.CreateString("Dublin"),
                        fbb.CreateString("1968-04-24"));
    obj = CreateCharacter(fbb, fbb.CreateString("Petyr Baelish"),
                          fbb.CreateString("Baelish"),
                          actor,
                          51, fbb.CreateString("Lord Snow"));
    fbb.Finish(obj);

    characters.push_back(
      Buffer(reinterpret_cast<char*>(fbb.GetBufferPointer()), // Buffer pointer
             fbb.GetSize()  // Buffer size
      )
    );

    db.MultiInsert("character",   // collection name in which to insert
                   characters     // data that is to be inserted
    );

    // Read data as resultset
    auto rs = db.ExecuteSelect("SELECT name, house, age "
                               "FROM character;");
    while (rs.Next()) {
      auto name = rs.GetString(rs.GetColumnIndex("name"));
      auto house = rs.GetString(rs.GetColumnIndex("house"));
      auto age = rs.GetInteger(rs.GetColumnIndex("age"));
    }

    // Read data with some query constraints 
    rs = db.ExecuteSelect("SELECT name, house, age "
                          "FROM character "
                          "WHERE age > 10 AND house = 'Stark';");
    while (rs.Next()) {
      auto name = rs.GetString(rs.GetColumnIndex("name"));
      auto house = rs.GetString(rs.GetColumnIndex("house"));
      auto age = rs.GetInteger(rs.GetColumnIndex("age"));
    }

    // Read data as the original document blob that was inserted
    rs = db.ExecuteSelect("SELECT _document FROM character;");
    while (rs.Next()) {
      auto doc = rs.GetBlob(rs.GetColumnIndex("_document"));      
    }

    // Read nested fields
    rs = db.ExecuteSelect("SELECT \"played_by.name\", \"played_by.date_of_birth\" "
                          "FROM character "
                          "WHERE \"played_by.name\" = 'Aidan Gillen';");
    while (rs.Next()) {
      auto actName = rs.GetString(rs.GetColumnIndex("played_by.name"));
      auto dob = rs.GetString(rs.GetColumnIndex("played_by.date_of_birth"));      
    }

  } catch (JonoonDBException& ex) {
    cout << ex.to_string() << endl;
    return 1;
  } catch (std::exception& ex) {
    cout << "Exception: " << ex.what() << endl;
    return 1;
  }

  return 0;
}



