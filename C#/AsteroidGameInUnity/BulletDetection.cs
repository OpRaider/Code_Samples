// attached to bullet prefab
// additional trigger detection
// also sets the bullet's tag to either EnemyProjectile or Projectile
// depending on who shot it.

using UnityEngine;
using System.Collections;

public class BulletDetection : MonoBehaviour {

	PlayerController player;

	bool isPlayerBullet;

	void Start() {
		player = GameObject.Find("Player").GetComponent<PlayerController>();
	}


	// called by the object that instantiates a bullet
	// sets the tag to player or enemy projectile
	// (called by UFOBehavior and PlayerController)
	public void SetTag(bool isPlayerProjectile) {
		tag = (isPlayerProjectile)? "Projectile" : "EnemyProjectile";
		isPlayerBullet = isPlayerProjectile;
	}

	void OnTriggerEnter2D(Collider2D col) {
		if (isPlayerBullet)
			playerBulletDetection(col);

		else 
			enemyBulletDetection(col);
	}

	void playerBulletDetection(Collider2D col) {
		if (col.tag == "Asteroid") {
			
			if (col.name != "SmallAsteroid(Clone)") {
				col.GetComponent<AsteroidBehavior>().SpawnSmallerAsteroids();	
			}
			
			if (col.name == "SmallAsteroid(Clone)")
				player.score += 100;
			
			else if (col.name == "MedAsteroid(Clone)")
				player.score += 50;
			
			else 	
				player.score += 20;
			
			GameObject.Find("BoomNoiseGO").GetComponent<AudioSource>().Play();
			Destroy(gameObject);
			Destroy(col.gameObject);
		}
		
		else if (col.tag == "Enemy") {
			if (col.name == "SmallUFO") 
				player.score += 1000;
			
			else
				player.score += 200;
			
			Destroy(gameObject);
			Destroy(col.gameObject);
		}
	}

	void enemyBulletDetection(Collider2D col) {
		if (col.tag == "Asteroid") {
			
			if (col.name != "SmallAsteroid(Clone)") {
				col.GetComponent<AsteroidBehavior>().SpawnSmallerAsteroids();	
			}
			
			GameObject.Find("BoomNoiseGO").GetComponent<AudioSource>().Play();
			Destroy(gameObject);
			Destroy(col.gameObject);
		}
		
		else if (col.tag == "Player") {

			GameObject.Find("BoomNoiseGO").GetComponent<AudioSource>().Play();
			Destroy(gameObject);
			player.LoseLife();
		}
	}
}
