// Attached to GameOver gameobject in Game scene

using UnityEngine;
using System.Collections;

public class GoFromGameToHighScores : MonoBehaviour {
	
	void Update () {
		if (Input.GetMouseButtonDown (0)) {
			GameObject.Find("Player").GetComponent<PlayerController>().SaveHighScore();
			Application.LoadLevel(2);
		}
	}
}
