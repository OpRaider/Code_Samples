/// <summary>
/// Attached to BigUFO and SmallUFO prefab
/// Controls the UFOs' movement and shooting behavior
/// </summary>

using UnityEngine;
using System.Collections;

public class UFOBehavior : MonoBehaviour {
	
	public GameObject bulletPrefab;

	PlayerController player;
	GameObject playerGO;

	bool isMovingRight, isMovingUp;

	void Start () {
		isMovingRight = (transform.position.x < 0)? true : false;
		isMovingUp = (transform.position.y < 0)? true : false;

		playerGO = GameObject.Find("Player");
		player = playerGO.GetComponent<PlayerController>();

		StartCoroutine(Move());		
	}

	// Starts UFO Movement sequence
	// .8 seconds after spawn, start shooting
	IEnumerator Move() {

		StartCoroutine(MoveX());
		StartCoroutine(MoveY());
		yield return new WaitForSeconds(.8f);
		StartCoroutine(Shoot());
		yield return null;
	}

	// If UFO spawned on right side, translate to the left, and vice versa
	IEnumerator MoveX() {
		Vector3 direction = (isMovingRight)? Vector3.right : Vector3.left;		
		
		float speed = (gameObject.name == "SmallUFO(Clone)")? 3.5f : 2.5f;

		while (Mathf.Abs(transform.position.x) < 11) {
			transform.Translate(direction * speed * Time.deltaTime);
			yield return null;
		}

		Destroy(gameObject);
		yield return null;
	}

	// if UFO spawned on the upper part of the screen
	// translate down, then back up
	// and vice versa
	IEnumerator MoveY() {
		Vector3 direction = (isMovingUp)? Vector3.up : Vector3.down;		
		Vector3 oppositeDir = (direction == Vector3.up)? Vector3.down : Vector3.up;

		float speed = (gameObject.name == "SmallUFO(Clone)")? 3.5f : 2.5f;

		float oneSecFromNow;
		while (true) {
			yield return new WaitForSeconds(.8f);
		
			oneSecFromNow = Time.time + 1;
			while (oneSecFromNow > Time.time) {
				transform.Translate(direction * speed * Time.deltaTime);
				yield return null;
			}
			yield return new WaitForSeconds(1);
			oneSecFromNow = Time.time + 1;
			while (oneSecFromNow > Time.time) {
				transform.Translate(oppositeDir * speed * Time.deltaTime);
				yield return null;
			}
			yield return null;
		}
	}

	IEnumerator Shoot() {
		while (true) {
			if(gameObject.name == "SmallUFO(Clone)") {
				ShootAtPlayer();
				yield return new WaitForSeconds(2);
			}
			else {
				RandomShoot();
				yield return new WaitForSeconds(.8f);
			}
			yield return null;
		}

		yield return null;
	}
	
	// Find a random position on a unit circle around the UFO
	// Shoot at that location
	void RandomShoot() {
		Vector3 spawnOffset = Random.insideUnitCircle.normalized * 10;
		int rand = Random.Range(0, 2);
		Vector3 direction = (rand == 0)? transform.TransformDirection(Vector3.up) : transform.TransformDirection(Vector3.down);
		
		Quaternion rotation = Quaternion.LookRotation
			(transform.position - spawnOffset, direction);
		
		rotation = new Quaternion(0, 0, rotation.z, rotation.w);
		GameObject bullet = Instantiate(bulletPrefab, transform.position, rotation) as GameObject;
		bullet.GetComponent<BulletDetection>().SetTag(false);
		bullet.rigidbody2D.AddRelativeForce(new Vector2(0,3) * 25);
		Destroy(bullet, .5f);
	}

	// Find player's postion, shoot at that position
	void ShootAtPlayer() {
		
		GameObject bullet = Instantiate(bulletPrefab, transform.position, Quaternion.identity) as GameObject;
		bullet.GetComponent<BulletDetection>().SetTag(false);

		bullet.transform.LookAt(playerGO.transform);
		bullet.rigidbody2D.AddForce(bullet.transform.forward * 75);
		
		bullet.transform.rotation = new Quaternion(0, 0, bullet.transform.rotation.z, bullet.transform.rotation.w);
		Destroy(bullet, .4f);
	}

	void OnTriggerEnter2D(Collider2D col) {
		if (col.tag == "Projectile") {
			if (gameObject.name == "SmallUFO(Clone)")
				player.score += 1000;
			
			else 	
				player.score += 200;
			
			GameObject.Find("BoomNoiseGO").GetComponent<AudioSource>().Play();
			Destroy(col.gameObject);
			Destroy(gameObject);
		}
		else if (col.tag == "Asteroid") {
			
			
			if (col.name != "SmallAsteroid(Clone)") {
				col.GetComponent<AsteroidBehavior>().SpawnSmallerAsteroids();	
			}
			
			GameObject.Find("BoomNoiseGO").GetComponent<AudioSource>().Play();
			Destroy(col.gameObject);
			Destroy(gameObject);
		}
	}
}
