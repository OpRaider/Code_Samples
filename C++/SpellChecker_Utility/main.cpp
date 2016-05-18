/* PROJECT DESCRIPTION (ADDED 5/16/2016): 

	Interactive CLI spell checker.
	Used to demonstrate OOP c++ skills.
*/

#include "SpellChecker.h"
#include "DocumentWorker.h"
#include <exception>
#include <stdexcept>
#include <iostream>

using namespace std;

void executeCommand(DocumentWorker& dw, SpellChecker& sc, string word)
{

    cout << "Wrong Spelling: " << word << '\n';

    bool invalid = true;
    while (invalid)
    {
        invalid = false;
        char command;
        cin >> command;

        switch (command)
        {
        case 'i': dw.ignoreCurrentWord(DocumentWorker::IGNORE_ONCE);    break;
        case 'I': dw.ignoreCurrentWord(DocumentWorker::IGNORE_ALL);    break;
        case 'a': dw.correctCurrentWord(word, true, true);            break;
        case 's': cin >> word;
                  dw.correctCurrentWord(word, false, false);         break;
        case 'S': cin >> word;
                  dw.correctCurrentWord(word, true, false);        break;
                case 'q':
                case 'Q': dw.addWordsTo_Dictionary();
                          dw.saveDocument();
                          sc.printTimeTaken();
                          sc.printFirstThreeWords();
                          exit(EXIT_SUCCESS);
        default:  invalid = true;
                cout << "Invalid command: " << command;        break;
        }
    }
}

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        cout << "Usage: Spell <FileName>\n";
        return EXIT_FAILURE;
    }
    
    try{
        DocumentWorker dw(argv[1]);
        SpellChecker sc;
        
        string currentWord;
        // while the next word isn't an empty string
        // aka the end of the document wasn't reached
        // side effect alert: stores the next word into local variable
        while (!(currentWord = dw.getNextWord()).empty())
        {
            if(!sc.isCorrect(currentWord))
            {
                executeCommand(dw, sc, currentWord); 
            }
            dw.updateCurrentWordCounter();
        }
        dw.addWordsTo_Dictionary();
        dw.saveDocument();
    }
    // can be thrown by DocumentWorker's and SpellChecker's constructors
    // also can be thrown by DocumentWorker::saveDocument()
    catch(ios_base::failure e)
    {
        cout << e.what() << '\n';
        cout << "Terminating program\n";
        return EXIT_FAILURE;    
    }
}
