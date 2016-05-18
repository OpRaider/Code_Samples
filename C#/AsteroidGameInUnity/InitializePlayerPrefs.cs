// Attached to the Title UISprite in MainMenu scene
// Initializes PlayerPrefs to default values if PlayerPrefs are not found
// Should only execute once per build (PlayerPrefs are lost upon building the game, I believe)

using UnityEngine;
using System.Collections;

public class InitializePlayerPrefs : MonoBehaviour {

	public void InitPlayerPrefs() {
		if (!PlayerPrefs.HasKey("Score10")) {
			
			for (int i = 1; i < 11; i++) {
				PlayerPrefs.SetInt("Score" + i.ToString(), -1);
				PlayerPrefs.SetString("Name" + i.ToString(), "empty");
			}
			PlayerPrefs.SetInt("MostRecentScore", -1);
			PlayerPrefs.SetString("MostRecentName", "");
		}
	}
}
