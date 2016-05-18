// Attached to Asteroid prefabs
// Controls asteroid behavior

using UnityEngine;
using System.Collections;

public class AsteroidBehavior : MonoBehaviour {
	public GameObject smallerAsteroidPrefab;	

	public Sprite style1, style2, style3, style4; // the different visual styles of asteroids

	PlayerController player;

	void Start () {
		// Randomly pick a visual style for this asteroid
		Sprite[] spriteArray = { style1, style2, style3, style4 };
		GetComponent<SpriteRenderer>().sprite = spriteArray[Random.Range(0, 4)];

		player = GameObject.Find("Player").GetComponent<PlayerController>();
	}

	// A big and medium asteroid spawns two smaller asteroids upon destruction
	public void SpawnSmallerAsteroids() {
		for (int i = 0; i < 2; i++) {
			GameObject smallerAsteroid = Instantiate(smallerAsteroidPrefab, transform.position, Quaternion.identity) as GameObject;
			
			Vector2 randomDirection = new Vector2(Random.Range(0f, 1f), Random.Range(0f, 1f));
			
			float force;
			if (smallerAsteroid.name == "SmallAsteroid(Clone)")
				force = 200;
			
			else
				force = 150;

			smallerAsteroid.rigidbody2D.AddForce(randomDirection * force);
		}
	}

	// If projectile collision, award points to player before destroying both bullet and asteroid
	// if enemy projectile, destroy bullet and asteroid
	void OnTriggerEnter2D(Collider2D col) {
		if (col.tag == "Projectile") {
			
			if (gameObject.name != "SmallAsteroid(Clone)") {
				SpawnSmallerAsteroids();	
			}

			if (gameObject.name == "SmallAsteroid(Clone)")
				player.score += 100;
			
			else if (gameObject.name == "MedAsteroid(Clone)")
				player.score += 50;
			
			else 	
				player.score += 20;

			GameObject.Find("BoomNoiseGO").GetComponent<AudioSource>().Play();
			Destroy(col.gameObject);
			Destroy(gameObject);
		}

		else if (col.tag == "EnemyProjectile") {
			
			if (gameObject.name != "SmallAsteroid(Clone)") {
				SpawnSmallerAsteroids();	
			}
			
			GameObject.Find("BoomNoiseGO").GetComponent<AudioSource>().Play();
			Destroy(col.gameObject);
			Destroy(gameObject);
		}
	}

	void Update() {
		// encountered a weird, rare bug where an asteroid escapes the playable boundaries and causes
		// the round to last forever, since that asteroid still hasn't been killed;
		// to solve it, the object is immediately destroyed if it escapes the boundaries.
		if (isOutofBounds())
			Destroy(gameObject);
	}

	bool isOutofBounds() {
		return (Mathf.Abs(transform.position.x) > 13 || Mathf.Abs(transform.position.y) > 9);
	}
}
