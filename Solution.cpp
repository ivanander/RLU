#include "Common.h"
#include <list>
#include <unordered_map>
#include <mutex>

using namespace std;

class LruCache : public ICache {
	using Id = string;
	list<Id> times;

	struct Entry {
		shared_ptr<IBook> book;
		list<Id>::iterator t;
	};

	mutex m;
	unordered_map<Id, Entry> cache;
	Settings settings;
	shared_ptr<IBooksUnpacker> books_unpacker;
	size_t cur_memory = 0;

public:
  LruCache(
      shared_ptr<IBooksUnpacker> books_unpacker,
      const Settings& settings
  ) :  settings(settings), books_unpacker(books_unpacker) {

  }

  BookPtr GetBook(const Id& book_name) override {
	  lock_guard<mutex> lg(m);

	  if (cache.find(book_name) == end(cache)) {
		  return DownLoad(book_name, move(books_unpacker->UnpackBook(book_name)));
	  } else {

		  times.erase(cache[book_name].t);
		  auto it = times.insert(times.end(), book_name);
		  cache[book_name].t = it;

		  return cache[book_name].book;
	  }
  }

  BookPtr DownLoad(const Id& book_name, unique_ptr<IBook> book) {

	  while ((cur_memory + book->GetContent().size() > settings.max_memory) && !times.empty()) {

		  cur_memory -= cache[times.front()].book->GetContent().size();
		  cache.erase(times.front());
		  times.pop_front();
	  }

	  if (cur_memory + book->GetContent().size() <= settings.max_memory) {

		  auto it = times.insert(times.end(), book_name);
		  cur_memory += book->GetContent().size();
		  cache[book_name] = Entry{move(book), it};

		  return cache[book_name].book;
	  } else {
		  return move(book);
	  }
  }
};


unique_ptr<ICache> MakeCache(
    shared_ptr<IBooksUnpacker> books_unpacker,
    const ICache::Settings& settings
) {
	return make_unique<LruCache>(books_unpacker, settings);

}
