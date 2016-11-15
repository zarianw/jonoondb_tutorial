#include <iostream>
#include <fstream>

#include "config_generated.h"
#include "schemas/character_generated.h"
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

Buffer ConstructCharacter(string name, string house, string playedBy,
                          int age, string firstSeen) {
  FlatBufferBuilder fbb;
  auto fbName = fbb.CreateString(name);
  auto fbHouse = fbb.CreateString(house);
  auto fbPlayedBy = fbb.CreateString(playedBy);
  auto fbFirstSeen = fbb.CreateString(firstSeen);  
  auto obj = CreateCharacter(fbb, fbName, fbHouse, fbPlayedBy, age, fbFirstSeen);
  fbb.Finish(obj);
  return Buffer(reinterpret_cast<char*>(fbb.GetBufferPointer()), fbb.GetSize());
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
    auto obj = CreateCharacter(fbb, fbb.CreateString("Tyrion Lannister"),
                               fbb.CreateString("Lannister"),
                               fbb.CreateString("Peter Dinklage"),
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
    obj = CreateCharacter(fbb, fbb.CreateString("Jon Snow"),
                          fbb.CreateString("Stark"),
                          fbb.CreateString("Kit Harington"),
                          21, fbb.CreateString("Winter is Coming"));
    fbb.Finish(obj);

    characters.push_back(
        Buffer(reinterpret_cast<char*>(fbb.GetBufferPointer()), // Buffer pointer
               fbb.GetSize()  // Buffer size
        )
    );

    fbb.Clear();
    obj = CreateCharacter(fbb, fbb.CreateString("Petyr Baelish"),
                          fbb.CreateString("Baelish"),
                          fbb.CreateString("Aidan Gillen"),
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

    // read data as resultset
    auto rs = 
      db.ExecuteSelect("SELECT name, house, age FROM character;");
    while (rs.Next()) {
      auto name = rs.GetString(rs.GetColumnIndex("name"));
      auto house = rs.GetInteger(rs.GetColumnIndex("house"));
      auto age = rs.GetInteger(rs.GetColumnIndex("age"));
    }

    // read data as the original document blob that was inserted
    rs = db.ExecuteSelect("SELECT _document FROM character;");
    while (rs.Next()) {
      auto doc = rs.GetBlob(rs.GetColumnIndex("_document"));      
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



