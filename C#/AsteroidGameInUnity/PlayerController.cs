// Attached to the player
// This script handles:
// player input, numlives, score, and the gameover overlay at end of game

using UnityEngine;
using System.Collections;

public class PlayerController : MonoBehaviour {

	public Sprite gameOverSprite, enterNameSprite, OKSprite;
	public GameObject bulletPrefab, lifePrefab, scoreOverlay;	

	Animator anim;
	AudioSource thrust, pew;
	TextMesh name, scoreOverlayText;

	
	public int numLives { get; set; }
	public int score { get; set; }

	int maxLives, numLivesGained;
	bool gameOver, readyfortext, respawning;

	void Start() {
		scoreOverlayText = GameObject.Find("ScoreOverlay").GetComponent<TextMesh>();
		name = GameObject.Find("Name").GetComponent<TextMesh>();
		gameOver = false;
		readyfortext = false;
		anim = GetComponent<Animator>();
		AudioSource[] sounds = GetComponents<AudioSource>();
		thrust = sounds[0];
		pew = sounds[1];
		numLives = 3;
		numLivesGained = 0;
		maxLives = 10;
		score = 0;
		thrust.mute = true;
		Respawn();
	}

	void Update () {
		scoreOverlayText.text = score.ToString();

		if ((score - (numLivesGained * 10000)) > 10000) // every 10,000 points, gain a life
			GainLife();

		if (!respawning && !gameOver) 
			Play();
			
		else if (readyfortext) // game over, get person's name for high score
			GetName();
	}
	
	void GainLife() {
		numLivesGained++;
		if (numLives < maxLives && numLives > 0) {
			var position = GameObject.FindGameObjectsWithTag("Life")[numLives-1].transform.position;
			position.x += .5f;
			numLives++;
			
			Instantiate(lifePrefab, position, Quaternion.identity);
		}
	}

	// Gets keyboard input, when enter is pressed, HighScores scene is loaded
	void GetName() {
		foreach (char c in Input.inputString) {
			if (c == "\b"[0]) {
				if (name.text.Length != 0)
					name.text = name.text.Substring(0, name.text.Length - 1);
			}
			else {
				if (c == "\n"[0] || c == "\r"[0]) {
					SaveHighScore();
					Application.LoadLevel(2);
				}
				else if (name.text.Length < 17)
					name.text += c;
			}
		}
	}

	public void SaveHighScore() {
		if (name.text != "") {
			PlayerPrefs.SetString("MostRecentName", name.text);
			PlayerPrefs.SetInt("MostRecentScore", score);
			PlayerPrefs.Save();
		}
	}
	
	void Play() {
		// move forward
		if (Input.GetKey(KeyCode.W) || Input.GetKey(KeyCode.UpArrow)) {
			anim.SetBool("Moving", true);
			thrust.mute = false;
			rigidbody2D.AddRelativeForce(new Vector3(0,6,0));
		}
		else {
			thrust.mute = true;
			anim.SetBool("Moving", false);
		}

		// rotate player
		if (Input.GetKey(KeyCode.A) || Input.GetKey(KeyCode.LeftArrow)) 
			transform.Rotate(new Vector3(0,0,1), 6);
		
		else if (Input.GetKey(KeyCode.D) || Input.GetKey(KeyCode.RightArrow)) 
			transform.Rotate(new Vector3(0,0,-1), 6);
		
		// shoot
		if (Input.GetKeyDown(KeyCode.Space)) {
			pew.Play();
			GameObject bullet = Instantiate(bulletPrefab, transform.position + transform.up * .25f, transform.rotation) as GameObject;
			bullet.GetComponent<BulletDetection>().SetTag(true);
			bullet.rigidbody2D.AddRelativeForce(new Vector2(0,3) * 25);
			Destroy(bullet, .6f);
		}
		
		// hyperspace (a random teleport)
		if (Input.GetKeyDown(KeyCode.R)) {
			var newPos = transform.position;
			newPos.x = Random.Range(-9.5f, 9.5f);
			newPos.y = Random.Range(-5f, 5f);
			
			transform.position = newPos;
		}
	}
	
	void OnTriggerEnter2D(Collider2D col) {
		if (col.tag == "Asteroid") {

			GameObject.Find("BoomNoiseGO").GetComponent<AudioSource>().Play();
			
			if (col.name != "SmallAsteroid(Clone)") {
				col.GetComponent<AsteroidBehavior>().SpawnSmallerAsteroids();	
			}

			if (col.name == "SmallAsteroid(Clone)")
				score += 100;

			else if (col.name == "MedAsteroid(Clone)")
				score += 50;
		
			else 	
				score += 20;
					
			LoseLife();

			Destroy(col.gameObject);
		}
		else if (col.tag == "Enemy") {

			GameObject.Find("BoomNoiseGO").GetComponent<AudioSource>().Play();

			if (col.name == "SmallUFO") 
				score += 1000;

			else
				score += 200;

			Destroy(col.gameObject);

			LoseLife();
		}
		else if (col.tag == "EnemyProjectile") {

			GameObject.Find("BoomNoiseGO").GetComponent<AudioSource>().Play();

			Destroy(col.gameObject);

			LoseLife();
		}
	}

	public void LoseLife() {
		Destroy(GameObject.FindGameObjectsWithTag("Life")[numLives-1]);
		if (--numLives > 0) 
			Respawn();
		
		else {
			Destroy(gameObject.renderer);
			Destroy(gameObject.collider2D);
			GameOver();
		}
	}

	public void GameOver() {
		gameOver = true;
		thrust.mute = true;
		StartCoroutine(GameOverCoroutine());	
	}
	
	// Sequence at end of game that displays that the game is over,
	// then 2 seconds later, changes the sprites to ask for their name
	public IEnumerator GameOverCoroutine() {
		GameObject GO = GameObject.Find("GameOver");
		var spriteRenderer = GO.GetComponent<SpriteRenderer>();
		spriteRenderer.sprite = gameOverSprite;

		yield return new WaitForSeconds(2);

		spriteRenderer.sprite = enterNameSprite;
		
		GO = GameObject.Find("OK");
		GO.GetComponent<SpriteRenderer>().sprite = OKSprite;
		GO.GetComponent<GoFromGameToHighScores>().enabled = true;

		readyfortext = true;

		yield return null;
	}

	// sets player to middle of screen
	// sets player velocity to zero
	// sets player rotation to identity
	// disables collider
	public void Respawn() {
		respawning = true;
		gameObject.collider2D.enabled = false;
		transform.position = new Vector2(0, 0);
		rigidbody2D.velocity = Vector2.zero;
		anim.SetBool("Moving", false);
		thrust.mute = true;
		transform.rotation = Quaternion.identity;
		
		StartCoroutine(RespawnCoroutine());
	}

	// Makes player blink every .2 seconds for 1.2 seconds
	// During this time, the player's collider is turned off
	// The player has .8 seconds to get to a safe spot before collider is turned back on
	IEnumerator RespawnCoroutine() {
		for (int i = 0; i < 6; i++) {
			if (i == 2)
				respawning = false;
			
			yield return new WaitForSeconds(.1f);
			gameObject.renderer.enabled = false;
			yield return new WaitForSeconds(.1f);
			gameObject.renderer.enabled = true;
		}

		gameObject.collider2D.enabled = true;

		yield return null;
	}
}
