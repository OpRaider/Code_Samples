using System;

namespace FogCreek
{
    /* Find a 9 letter string of characters that contains only letters from
     * acdegilmnoprstuw
     * such that the hash(the_string) is
     * 945924806726376
	 * 
	 * My Solution's worst case runtime complexity: O(N^2 * M) where N = length of the_string and M = length of the set of letters.
     */
    public class JobApp
    {
        // read only constants
        static Int64 ANSWER = 945924806726376;
        static string letters = "acdegilmnoprstuw";

        public static void Main(string[] args)
        {
            string solution = findSolution();

            Console.WriteLine("Solution is correct: " + Equals(hash(solution), ANSWER));
            Console.WriteLine("Solution is: " + solution);
        }


        public static string findSolution()
        {
            // init hashcode and iterators
            Int64 hashcode = 0;
            int startIdx = 0;
            char next = 'c';
            int nextIdx = 1;
            int prevIdx = 0;

            // init solution string to the smallest possible hashcode result
            string startStr = "aaaaaaaaa";                   
            char[] solution = startStr.ToCharArray();
            

            while (hashcode != ANSWER)
            {

                // change the most recently changed letter to 1 letter behind current letter (that's in the letters set).
                // ex: promeoaaaa, o was most recently changed. change o to n, new solution = promenaaaa
                // change hashcode to 0 to force into the next while body.
                if (hashcode > ANSWER)
                {
                    solution[startIdx - 1] = letters[prevIdx];
                    hashcode = 0;
                }

                /* Changes the solution string, starting with startIdx, with the next character in the set, until hashcode is greater than or equal to answer
                 * ex: startIdx = 0
                 * iteration 0: solution = "aaaaaaaaa"
                 * iteration 1: solution = "ccccccccc"
                 * iteration 11: solution = "rrrrrrrrr"
                 * etc; startIdx = 2
                 * iteration 0: solution = "praaaaaaa"
                 * iteration 1: solution = "prccccccc"
                 * iteration 9: solution = "prooooooo"
                 */
                while (hashcode < ANSWER)
                {
                    prevIdx = Math.Abs((nextIdx - 1) % letters.Length);

                    for (int i = startIdx; i < startStr.Length; i++)
                        solution[i] = next;
                    
                    nextIdx = (nextIdx + 1) % letters.Length;
                    next = letters[nextIdx];
                    
                    hashcode = hash(new string(solution));
                }

             

                if (hashcode == ANSWER)
                    break;

                // reset solution from startIdx + 1 to end, to smallest possible answer
                // ex: solution = promenbbb -> promenbaa
                for (int i = startIdx + 1; i < startStr.Length; i++)
                    solution[i] = 'a';

                // rehash and reset/update iterators
                hashcode = hash(new string(solution));
                startIdx++;
                next = 'a';
                nextIdx = 0;
                
            }

            return new string(solution);
        }


        public static Int64 hash(string s)
        {
            Int64 h = 7;
            // letters has been declared as a global constant up above so we don't reinitialize a string every time we call this function

            for (Int32 i = 0; i < s.Length; i++)
            {
                h = (h * 37 + letters.IndexOf(s[i]));
            }
            return h;
        }
    }
}
