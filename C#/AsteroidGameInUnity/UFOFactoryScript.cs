/// <summary>
/// Attached to empty game object in game scene
/// Spawns a UFO at a particular spawn location every 1100 points earned from player
/// UFO order: Big UFO, Small UFO, Big UFO, Small UFO, etc...
/// until 10 UFOs have spawned; then only spawn small UFOs.
/// </summary>

using UnityEngine;
using System.Collections;

public class UFOFactoryScript : MonoBehaviour {
	
	public GameObject bigUFO, smallUFO;

	PlayerController player;

	int numUFOsSpawned;
	bool spawnSmallUFO;

	void Start () {
		numUFOsSpawned = 0;
		spawnSmallUFO = false;
		player = GameObject.Find("Player").GetComponent<PlayerController>();
	}
	
	void Update () {
		if ((player.score - (numUFOsSpawned * 1100)) > 1100) { // spawn a UFO every 1100 points
			SpawnUFO();
		} 
	}

	void SpawnUFO() {
		numUFOsSpawned++;
		
		GameObject UFOtoSpawn = spawnSmallUFO? smallUFO : bigUFO;

		Transform[] spawnLocations = GetComponentsInChildren<Transform>();
						
		var spawnLocation = spawnLocations[Random.Range(1, 13)].position;
		spawnLocation.z = 0;
		GameObject UFO = Instantiate(UFOtoSpawn, spawnLocation, Quaternion.identity) as GameObject;
		
		if (numUFOsSpawned < 10) // spawn only small UFOs from the 10th spawned UFO onwards
			spawnSmallUFO = !spawnSmallUFO;
	}
}
