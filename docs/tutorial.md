# Tutorial
This tutorial assumes that you are familiar with serialization library [flatbuffers](http://google.github.io/flatbuffers/) and know how to use it. The complete code for this tutorial is available on [github](https://github.com/zarianw/jonoondb_tutorial).
### Prerequisite
* You would need the flatbuffers compiler flatc and its header files. You can build it from sources by following directions at this [link](http://google.github.io/flatbuffers/flatbuffers_guide_building.html). 
* You would need the [JonoonDB library](https://github.com/zarianw/jonoondb).

### Open a database
A JonoonDB database has a name which corresponds to a file system directory. All of the contents of database are stored in this directory. The following example shows how to open a database, creating it if necessary:
```c++
#include "jonoondb/jonoondb_api/database.h"

using namespace std;
using namespace flatbuffers;
using namespace jonoondb_tutorial;
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
In JonoonDB collections are like tables. A given database can have 1 or more collections. Each collection also has a schema which specifies the collection fields and their types. Currently JonoonDB only supports flatbuffers schema type.
#### Schema
Lets look at the flatbuffers schema that we will use for our tutorial.
```
namespace jonoondb_tutorial;

table Actor {
  name: string;
  date_of_birth: string;
  birth_city: string;
}

table Character {
  name:string;
  house:string;  
  played_by:Actor;
  age:int;
  first_seen:string;  
}

root_type Character;
```
Before proceeding any further we need to generate few files using the flatbuffers compiler flatc. Save the schema shown above in a text file and name it character.fbs. Next compile this file using the following command.
```
flatc -c -b --schema character.fbs
```
This will generate a header file **character_generated.h** and binary flatbuffers schema file **character.bfbs**. The character_generated.h file has helper functions to generate flatbuffers object for Character type. The chracter.bfbs has the same schema shown above but in binary form. This is required because internally JonoonDB uses flatbuffers reflection mechanism to read flatbuffer objects and for reflection we need the binary schema.

The code below reads the binary flatbuffers schema from the file characters.bfbs and creates a collection.
```c++
auto schema = ReadFile("path/to/character.bfbs");
vector<IndexInfo> indexes;
db.CreateCollection("character",                  // collection name   
                    SchemaType::FLAT_BUFFERS,     // collection schema type
                    schema,                       // collection schema
                    indexes                       // indexes to create
);
```
The indexes parameter specifies the indexes that should be created for this collection. The use of indexes is covered in its own sections later in the tutorial. For now we will not create any indexes and pass an empty vector. ReadFile() is just reading the entire binary file and returning its contents. You can look at its implementation [here](https://github.com/zarianw/jonoondb_tutorial/blob/master/main.cc#L13).
### Insert single document
First we will construct a flatbuffer object. We are using the CreateActor() and CreateCharacter() functions that were generated by the flatc compiler inside character_generated.h. Next we construct a object of type Buffer that we pass to the db.Insert() function.
```c++
#include "character_generated.h"

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
```
One important thing to note here is how the Buffer object "tyrion" was constructed. Buffer class objects can be of two types. They either own the underlying memory in which case they will delete the underlying buffer on destruction. The second type is just a view on top of some memory and on destruction it does not delete any memory. Here we are using the latter type by specifying the function pointer to deleter as nullptr. 
### Insert multiple documents / Bulk Insert
The MultiInsert() function is optimized for loading large number of documents into the database. It is way faster than Insert() function and should be the preferred way to load data into the database. 
```c++
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
```
Note that here we are constructing the Buffer using a different constructor overload. This type of Buffer has its own underlying memory that it will delete on destruction. This was necessary here because we are reusing the FlatbufferBuilder object fbb. We do that by calling fbb.clear() before constructing every new character. Hence the underlying fbb memory is no longer valid after this call, so we create a copy of it before the clear() call.

There are other more optimized ways to go about this as well for example instead of creating new object of type Buffer, you can have a reusable pool of Buffer objects. Another way could have been that you use a different FlatbufferBuilder object to construct each character and then use the technique we used above in the Insert() call. Remember we created the Buffer by specifying the deleter function pointer as nullptr. The approach that will work best for you depends on your application but all of these could be a viable solution depending on your needs.
### Querying data
JonoonDB supports querying the documents using SQL. Consider the following example:
```c++
auto rs = db.ExecuteSelect("SELECT name, house, age "
                           "FROM character;");
while (rs.Next()) {
  auto name = rs.GetString(rs.GetColumnIndex("name"));
  auto house = rs.GetString(rs.GetColumnIndex("house"));
  auto age = rs.GetInteger(rs.GetColumnIndex("age"));
}
```
ExecuteSelect() function can be used to issue SELECT statements. The functions returns a Resultset object. The rs.Next() moves to the next document in the resultset and will keep returning true until there are more documents available in the resultset. 

Here is another example where we are using the some query constraints:
```c++
rs = db.ExecuteSelect("SELECT name, house, age "
                      "FROM character "
                      "WHERE age > 10 AND house = 'Stark';");
while (rs.Next()) {
  auto name = rs.GetString(rs.GetColumnIndex("name"));
  auto house = rs.GetString(rs.GetColumnIndex("house"));
  auto age = rs.GetInteger(rs.GetColumnIndex("age"));
}
```
#### Getting the raw documents
The queries written above gives you the data as structured resultset. What if you want to get back the raw document blob that you inserted? Each collection has a virtual hidden column named **_document**. When used in a query this evaluates to the raw document blob that was originally inserted. For example:
```c++
rs = db.ExecuteSelect("SELECT _document FROM character;");
while (rs.Next()) {
  auto doc = rs.GetBlob(rs.GetColumnIndex("_document"));      
}
```
This will return the original document that we inserted.
#### Querying/Accessing nested fields
JonoonDB supports nested fields as well. In our schema we have a nested field of type **Actor**. Lets look at an example where we want to retrieve a nested field and also filter the results based on its value.
```c++
rs = db.ExecuteSelect("SELECT \"played_by.name\", \"played_by.date_of_birth\" "
                      "FROM character "
                      "WHERE \"played_by.name\" = 'Aidan Gillen';");
while (rs.Next()) {
  auto actName = rs.GetString(rs.GetColumnIndex("played_by.name"));
  auto dob = rs.GetString(rs.GetColumnIndex("played_by.date_of_birth"));      
}
```
The important thing to note here is that the nested fields are **dot separated** and **enclosed in double quotes ""**. Hence in the above example we wrote **"played_by.name"** and **"played_by.date_of_birth"**. This is the only difference other than that you can use them in SQL like any other field. The above query in plain text is shown below:
```sql
SELECT "played_by.name", "played_by.date_of_birth"
FROM character
WHERE "played_by.name" = 'Aidan Gillen';
```
#### Querying Date and Time: TODO

#### Indexing
Indexing is the mechanism that enables JonoonDB to do efficient lookups, aggregations and sorting. Without indexes JonoonDB must do a full collection scan i.e scan each and every document in the collection. JonoonDB's approach to indexing is very different to traditional databases. This is what sets it apart from the pack as well.  

In traditional databases the practical number of indexes that can be created on a table is very low. The reason is simple as you create more that few indexes the insert performance becomes so slow that the whole database comes to a halt. In JonoonDB all indexes are maintained in memory using really fast in-memory data structures. This enables JonoonDB to maintain fast insert performance even with dozens of indexes. This of-course means that your indexes should fit in memory for good performance but that is a requirement that you will find in all leading databases.

The second big difference is how JonoonDB query planner can use multiple indexes in a single query plan. A number of leading databases can only use 1 index in a given query plan. JonoonDB can use multiple indexes in a single query plan.

The third difference is the extensible indexing design in JonoonDB. JonoonDB is designed from the grounds up where different type of indexes can be developed and added. This means that more and more index implementations will be added in future. This gives a tremendous amount of flexibility and you as a user can write your own index implementation if you want. You will never be locked in to what a given database provides. This is one of the important features that enables JonoonDB to be a one size fits all database. 

The following index implementations exist in JonoonDB.
1. **InvertedCompressedBitmap:** In this data structure. The values are mapped to the document ids in which they exist. For example if you have a field State then value such as CA will be mapped to the document ids which have field State = CA. Hence the word inverted. The document ids are stored as compressed bitmap which enables huge space saving if used on the right field. Further all values are stored in a sorted tree data structure which enables efficient range based lookups. For example if you have a column Age and you want to do a query like Age > 10 and Age < 20. You should almost always use this index type for low cardinality (less than 50K distinct values but do your own testing) columns but even with high cardinality columns they can yield superior performance. Here is a [link](http://lemire.me/blog/2008/08/20/the-mythical-bitmap-index/) to an article that talks about bitmap indexes and offers good advice and benchmarks.
2. **Vector:** This index data structure is a simple vector (array) of values as they exist in the documents. The index of the vector is the document id and content at that index location is the actual value. This is a good default if you want to arrange your data as a column store. The column oriented data results in really fast scans and aggregations. Its also much faster to insert data in this data structure.

The following code snippet builds on top of the tutorial (add link) example and shows how to create an index.

```c++
vector<IndexInfo> indexes;
indexes.push_back(IndexInfo("idx_name",        // Name
                            IndexType::VECTOR, // Type
                            "age",             // Indexed Field
                            true)              // IsAscending
);
indexes.push_back(IndexInfo("idx_age",    // Name
                            IndexType::INVERTED_COMPRESSED_BITMAP, // Type
                            "age",        // Indexed Field
                            true)         // IsAscending
);

db.CreateCollection("character",                  // collection name   
                    SchemaType::FLAT_BUFFERS,     // collection schema type
                    schema,                       // collection schema
                    indexes                       // indexes to create
);
```

