# Tutorial

### Open a database
A JonoonDB database has a name which corresponds to a file system directory. All of the contents of database are stored in this directory. The following example shows how to open a database, creating it if necessary:
```c++
#include "jonoondb/jonoondb_api/database.h"
using namespace std;
using namespace jonoondb_api;

// Open a database with default options
Database db("/path/to/db",     // path where db files will be created
            "game_of_thrones"  // database name              
);
```
If you want to raise an error if the database already exists, use the other constructor overload of jonoondb_api::Database class.
```c++
Options opt;
opt.SetCreateDBIfMissing(false);
Database db("/path/to/db",     // path where db files will be created
            "game_of_thrones", // database name 
            opt                // options             
);
```
### Create a collection
In JonoonDB collections are like tables. A given database can have 1 or more collections. Each collection also has a schema which specifies the collection fields and their types. Currently JonoonDB only supports flatbuffers schema type. The code below read the binary flatbuffers schema from the file characters.bfbs and creates a collection.
```c++
auto schema = ReadFile("path/to/character.bfbs");
vector<IndexInfo> indexes;
db.CreateCollection("character",                  // collection name   
                    SchemaType::FLAT_BUFFERS,     // collection schema type
                    schema,                       // collection schema
                    indexes                       // indexes to create
);
```
The indexes parameter specifies the indexes that should be created for this collection. The use of indexes is covered in its own sections later in the tutorial. For now we will not create any indexes and pass an empty vector.
### Insert single document
First we will construct a flatbuffer object. The function ConstructCharacter is doing that and returning a Buffer. We pass this buffer to the db.Insert() function.
```C++
#include "schemas/character_generated.h"
using namespace flatbuffers;
using namespace jonoondb_tutorial;

FlatBufferBuilder fbb;
auto obj = CreateCharacter(fbb, fbb.CreateString("Tyrion Lannister"),
                           fbb.CreateString("Lannister"),
                           fbb.CreateString("Peter Dinklage"),
                           39, fbb.CreateString("Winter is Coming"));
fbb.Finish(obj);
Buffer tyrion(reinterpret_cast<char*>(fbb.GetBufferPointer()),
              fbb.GetSize());
db.Insert("character",   // collection name in which to insert
          tyrion         // data that is to be inserted
);
```
The function ConstructCharacter is using the flatbuffer functions defined in character_generated.h which was generated earlier. The 


