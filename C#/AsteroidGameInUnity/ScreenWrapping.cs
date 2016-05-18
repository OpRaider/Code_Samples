/// <summary>
/// Attached to every moving object in the game scene
/// If the moving game object collides with a game edge
/// it is teleported to the opposite edge
/// </summary>

using UnityEngine;
using System.Collections;

public class ScreenWrapping : MonoBehaviour {

	void OnTriggerEnter2D(Collider2D c) {
		var newPos = transform.position;

		if (c.name == "LeftEdge")
			newPos.x = -newPos.x - .5f;
		
		else if (c.name == "RightEdge")
			newPos.x = -newPos.x + .5f;

		else if (c.name == "BottomEdge")
			newPos.y = -newPos.y - .5f;

		else if (c.name == "TopEdge")
			newPos.y = -newPos.y + .5f;

		transform.position = newPos;
		
	}

}
