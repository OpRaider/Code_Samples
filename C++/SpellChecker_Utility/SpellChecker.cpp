/* PROJECT DESCRIPTION (ADDED 5/16/2016): 

	Interactive CLI spell checker.
	Used to demonstrate OOP c++ skills.
*/

#include "SpellChecker.h"

using namespace std;

SpellChecker::SpellChecker()
{
    ifstream file("/usr/share/dict/words");
    string line;

    // open file and store to set; terminate program if file doesn't open correctly
    if (file.is_open())
    {
        typedef chrono::high_resolution_clock Time;
        auto begin = Time::now();
        
        while (getline(file, line))
        {
            line[0] = tolower(line[0]);     // ABC and aBC are treated as equal, 
                                            // so to prevent searching for both ABC and aBC
                                            // all entries in the dictionary 
                                            // will have their first char lowered when loaded
            stdDict.push_back(line);
        }
        auto end = Time::now();
        file.close();
        timeTaken = end - begin;
    }
    else
    {
        throw ios_base::failure("Unable to open: /usr/share/dict/words");
    }

    file.open((getenv("HOME") + string("/_Dictionary")).c_str());

    if (file.is_open())
    {
        while (getline(file, line))
        {
            line[0] = tolower(line[0]);
            userDict.push_back(line);
        }
        file.close();
    }
    // second file is optional, thus do nothing if it can't open
}

void SpellChecker::printTimeTaken() const
{
    // timeTaken is a duration in milliseconds, so divide by 1000 to get seconds
    cout << timeTaken.count() / 1000.0 << endl;
}

void SpellChecker::printFirstThreeWords()
{
    DoublyLinkedList<string>::iterator it = stdDict.begin();
    
    for (int i = 0; i < 3; i++)
        cout << *(it++) << endl;
}

bool SpellChecker::isCorrect(string word)
{
    word[0] = tolower(word[0]);
    
    DoublyLinkedList<string>::iterator end = stdDict.end();
    DoublyLinkedList<string>::iterator it = stdDict.find(word);
    if (it != end)
        return true;
    
    it = userDict.find(word);

    return it != end;
}
