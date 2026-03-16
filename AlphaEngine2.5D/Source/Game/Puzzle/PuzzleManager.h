#ifndef PUZZLEMANAGER_H
#define PUZZLEMANAGER_H
#include"PuzzleObject.h"
#include <vector>

class PuzzleManager
{
public:
	static std::vector<Book> allBooks;
	static void CreatePuzzleObject(Book book);
	static void BookDataInit();
	static std::vector<Book>Books;
	static void Init();
	static void FreeManager();
	static int booksCollected;
	static std::string activeBookText;
	static float popupTimer;
	static std::string activeBookTitle;
	static void OpenExitDoor();


};

#endif 