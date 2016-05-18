/* PROJECT DESCRIPTION (ADDED 5/16/2016): 

	Interactive CLI spell checker.
	Used to demonstrate OOP c++ skills.
*/

#include "DocumentWorker.h"
#define ITERATOR DoublyLinkedList<string>::iterator
#define AUTO_ITERATOR DoublyLinkedList<pair<string, string> >::iterator

using namespace std;

DocumentWorker::DocumentWorker(string filename)
{
    documentName = filename;    // init name 
    // open file and store to list; terminate program if file doesn't open correctly
    ifstream file(filename.c_str());
    if (file.is_open())
    {
        char character = file.get();
        while (!file.eof())
        {
            string word;

            // groups up spaces, characters, and punctuation into their own string groups
            /* ex:  "   This is an example."
            for that input, the list will look like this:
            "   ", "This", " ", "is", " ", "an", "example", ".", "\n"  */
            if (isspace(character))
            {
                while (isspace(character) && !file.eof())
                {
                    word.push_back(character);
                    character = file.get();
                }
            }
            else if (ispunct(character) && character != '\'')
            {
                while (ispunct(character) && character != '\'' && !file.eof())
                {
                    word.push_back(character);
                    character = file.get();
                }
            }
            else
            {
                while ((isalnum(character) || character == '\'') && !file.eof())
                {
                    word.push_back(character);
                    character = file.get();
                }
            }

            document.push_back(word);
            word.clear();
        }
        file.close();
        currentWord = document.begin();    // init currentWord
    }
    else
    {
        throw ios_base::failure("Unable to open: " + filename);
    }
}

bool DocumentWorker::findAndCorrectCurrentWord(string word)
{
    // if word is found, autoCorrect word and return true
    AUTO_ITERATOR wordFound = autoCorrectedWords.findPair(word);
    AUTO_ITERATOR end = autoCorrectedWords.end();
    if (wordFound != end)
    {
        correctCurrentWord(autoCorrectedWords[word], false, false);
        return true;
    }
    else return false;
}

string DocumentWorker::getNextWord()
{
    ITERATOR end = document.end();
    if (currentWord == end)
        return "";

    // while the first character in the current word is not a letter or number
    // or if the current word is in ignoredWords
    // or if the current word is in autoCorrectedWords, update counter
    // has a side effect: autocorrects the current word if it's found in autoCorrectedWords
    ITERATOR it = ignoredWords.find(*currentWord);
    while (!isalnum((*currentWord).at(0))
        || it != end
        || findAndCorrectCurrentWord(*currentWord))
    {
        currentWord++;
        if (currentWord == end)
            return "";
        it = ignoredWords.find(*currentWord);
    }

    return *currentWord;
}

int DocumentWorker::getSize() const
{
    return document.size();
}

void DocumentWorker::correctCurrentWord(string correctedWord, bool correctAll, bool addToDict)
{
    if (!correctAll && addToDict)
    {
        cerr << "WARNING: It makes no sense for correctAll to be false while addToDict is true!\n"
            << "WARNING: Word will NOT be added to dictionary!\n";
    }

    if (correctAll)
    {
        pair<string, string> aPair(*currentWord, correctedWord);
        autoCorrectedWords.insert(aPair);
        if (addToDict)
        {
            wordsToBeAddedToDict.push_back(*currentWord);
        }
    }

    *currentWord = correctedWord;
}

void DocumentWorker::updateCurrentWordCounter()
{
    currentWord++;
}

void DocumentWorker::ignoreCurrentWord(bool ignoreAll)
{
    if (ignoreAll)
    {
        ignoredWords.insert(*currentWord);
    }
}

void DocumentWorker::saveDocument()
{
    ofstream file(documentName.c_str(), ios::trunc);

    if (file.is_open())
    {
        ITERATOR it = document.begin();
        ITERATOR end = document.end();
        for (; it != end; it++)
            file << *it;

    }
    else
        throw ios_base::failure("Unable to open: " + documentName);
}

void DocumentWorker::addWordsTo_Dictionary()
{
    string userDictionaryPath = getenv("HOME") + string("/_Dictionary");
    ofstream file(userDictionaryPath.c_str(), ios::app);

    if (file.is_open())
    {
        ITERATOR it = wordsToBeAddedToDict.begin();
        ITERATOR end = wordsToBeAddedToDict.end();
        for (; it != end; it++)
            file << *it << '\n';

        file.close();
    }
    else
        throw ios_base::failure("Unable to open: " + userDictionaryPath);

}



