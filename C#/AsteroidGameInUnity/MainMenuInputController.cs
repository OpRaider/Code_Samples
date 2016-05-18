// Attached to PlayGame and HighScores UISprites in MainMenu scene
// Start() is executed once, by PlayGame UISprite
// If PlayGame is pressed, BeginGame() is called
// if HighScores is clicked, GoToScore() is called

using UnityEngine;
using System.Collections;
using Holoville.HOTween;

public class MainMenuInputController : MonoBehaviour {

	public UISprite Title;
	public UISprite PlayGame;
	public UISprite HighScores;
	
	void Start () {	
		if (gameObject.name == "PlayGame") {
			HOTween.From(Title.transform, 4f, "localScale", Vector3.zero);
			HOTween.From(PlayGame.transform, 4f, "localScale", Vector3.zero);
			HOTween.From(HighScores.transform, 4f, "localScale", Vector3.zero);
		}
	}

	void BeginGame() {
		Application.LoadLevel(1);
	}
	
	void GoToScores() {
		Application.LoadLevel(2);
	}
}
