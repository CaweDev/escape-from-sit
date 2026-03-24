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
	static bool isQuizActive;
	static int currentCorrectAnswer;
	static Engine::GameObject* currentBookObj;
	static int currentBookId;
	static std::string activeOpt1;
	static std::string activeOpt2;
	static std::string activeOpt3;

};

#endif 