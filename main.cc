#include <iostream>
#include <string>
#include <fstream>
#include <exception>

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

Buffer ConstructCharacter(string name, string house, VitalStatus sts,
                          string playedBy, int age, string firstSeen,
                          vector<string>& aliases) {
  FlatBufferBuilder fbb;
  auto fbName = fbb.CreateString(name);
  auto fbHouse = fbb.CreateString(house);
  auto fbPlayedBy = fbb.CreateString(playedBy);
  auto fbFirstSeen = fbb.CreateString(firstSeen);
  auto fbAliases = fbb.CreateVectorOfStrings(aliases);
  auto obj = CreateCharacter(fbb, fbName, fbHouse, sts, fbPlayedBy, age, fbFirstSeen, fbAliases);
  fbb.Finish(obj);
  return Buffer(reinterpret_cast<char*>(fbb.GetBufferPointer()), fbb.GetSize());
}

int main(int argc, char** argv) {
  try {
    // Open a database with default options
    Database db(DB_PATH,           // path where db files will be created
                "game_of_thrones"  // database name              
    );

    // Read the binary flatbuffer schema from file.
    // This defines the collection schema.
    string schemaFolder = SCHEMA_PATH;
    auto schema = ReadFile(schemaFolder + "/character.bfbs");

    db.CreateCollection("character",                  // collection name   
                        SchemaType::FLAT_BUFFERS,     // collection schema type
                        schema                        // collection schema
    );

    std::vector<string> aliases = { "The Imp", "Halfman", "The Little Lion", "Demon Monkey", "The Bloody Hand" };
    auto tyrion = ConstructCharacter("Tyrion Lannister", "Lannister", VitalStatus::VitalStatus_Alive, "Peter Dinklage", 39, "Winter is Coming", aliases);

    // Insert data into db
    db.Insert("character",   // collection name in which to insert
              tyrion         // data that is to be inserted
    );


    std::vector<Buffer> characters;

    aliases.clear();
    aliases = { "Lord Snow", "The Bastard of Winterfell", "King Crow", "The Prince That Was Promised", "The White Wolf" };
    characters.push_back(ConstructCharacter("Jon Snow", "Stark", VitalStatus::VitalStatus_Alive, "Kit Harington", 21, "Winter is Coming", aliases));

    aliases.clear();
    aliases = { "Littlefinger" };
    characters.push_back(ConstructCharacter("Petyr Baelish", "Baelish", VitalStatus::VitalStatus_Alive, "Aidan Gillen", 51, "Lord Snow", aliases));

    db.MultiInsert("character",   // collection name in which to insert
                   characters     // data that is to be inserted
    );

  } catch (JonoonDBException& ex) {
    cout << ex.to_string() << endl;
    return 1;
  } catch (std::exception& ex) {
    cout << "Exception: " << ex.what() << endl;
    return 1;
  }

  return 0;
}



