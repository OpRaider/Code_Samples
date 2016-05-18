// Attached to highscores panel in HighScores scene
// Places most recent score in player prefs if it's a high score
// Sorts the scores
// Displays the scores

using UnityEngine;
using System.Collections;

public class DisplayHighScores : MonoBehaviour {
	
	void ToMainMenu() {
		Application.LoadLevel(0);
	}
	
	void Start() {
		PlaceScoreInOrder();
		DisplayScores();
		PlayerPrefs.Save();
	}
	
	// Find where to put most recent score in high scores if it belongs, and put it there
	void PlaceScoreInOrder() {
		
		int mostRecentScore = PlayerPrefs.GetInt("MostRecentScore");
		
		for (int i = 1; i < 11; i++) {
			int highScore = PlayerPrefs.GetInt("Score" + i.ToString());
			if (mostRecentScore > highScore) {
				PlayerPrefs.SetInt("Score" + i.ToString(), mostRecentScore);
				string highScorersName = PlayerPrefs.GetString("Name" + i.ToString());
				PlayerPrefs.SetString("Name" + i.ToString(), PlayerPrefs.GetString("MostRecentName"));
				ShiftScoresDown(highScorersName, highScore, i);
				break;
			}
		}

		// recent most recent score and name, so it's placed again, on highscores reload
		PlayerPrefs.SetInt("MostRecentScore", -1);
		PlayerPrefs.SetString("MostRecentName", "");
	}
	
	// recursive function that takes a name and score
	// gets the name and score from one down the list
	// sets the current name and score to the one in parameters
	// calls itself with the name and score from one down the list
	// base case: return when Score10 is reached; only top10 is stored
	void ShiftScoresDown(string name, int score, int i) {
		
		if (i == 10)
			return;
		
		string oneNameDown = PlayerPrefs.GetString("Name" + (i + 1).ToString());
		int oneScoreDown = PlayerPrefs.GetInt("Score" + (i + 1).ToString());
		
		i++;
		PlayerPrefs.SetString("Name" + i.ToString(), name);
		PlayerPrefs.SetInt("Score" + i.ToString(), score);
		
		ShiftScoresDown(oneNameDown, oneScoreDown, i);
	}	
	
	// Display the top ten scores
	void DisplayScores() {
		for (int i = 1; i < 11; i++) {
			int highScore = PlayerPrefs.GetInt("Score" + i.ToString());
		
			if (highScore > 0) {
				string highScorersName = PlayerPrefs.GetString("Name" + i.ToString());
				
				GameObject line = GameObject.Find("HighScoreLine" + i.ToString());
				line.transform.FindChild("Name").GetComponent<TextMesh>().text = highScorersName;
				line.transform.FindChild("Score").GetComponent<TextMesh>().text = highScore.ToString();
			}
		}
	}
}
