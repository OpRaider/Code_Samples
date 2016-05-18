// Attached to empty gameobject in scene
// Spawns asteroids at particular spawn points
// if there is no asteroids found
// First round = 4 asteroids spawn
// Every round after, spawn an additional asteroid
// So, 1st = 4, 2nd = 5, 3rd = 6, etc etc...

using UnityEngine;
using System.Collections;

public class AsteroidFactoryScript : MonoBehaviour {

	public GameObject asteroidPrefab;

	public float startingSpeed;
	public float speedIncreasePerRound;
	public float maxSpeed;


	public int roundNum { get; set; }

	float speed;
	int numAsteroidsToSpawn;
	

	void Start () {
		roundNum = 0;
		speed = startingSpeed;
		numAsteroidsToSpawn = 4;
	}
	
	public void SpawnAsteroids() {
		roundNum++;
		Transform[] spawnLocations = GetComponentsInChildren<Transform>();
		for (int i = 0; i < numAsteroidsToSpawn; i++) {
			var spawnLocation = spawnLocations[Random.Range(0, 25)].position; // pick a random spawn location
			spawnLocation.z = 0;

			GameObject asteroid = Instantiate(asteroidPrefab, spawnLocation, Quaternion.identity) as GameObject;
			asteroid.rigidbody2D.velocity = new Vector2(speed * Random.Range(0f, 1f), speed * Random.Range(0f, 1f)); 
		}
		numAsteroidsToSpawn++;
		if (speed < maxSpeed)
			speed += speedIncreasePerRound;
	}

	void Update () {
		GameObject[] asteroids = GameObject.FindGameObjectsWithTag("Asteroid");
		if (asteroids == null || asteroids.Length == 0) {	
			SpawnAsteroids();
		}
	}
}
