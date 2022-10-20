<script>
	//import Globe from 'globe.gl';
	import { browser } from '$app/env';
	import Play from "carbon-icons-svelte/lib/Play.svelte";
	import Pause from "carbon-icons-svelte/lib/Pause.svelte";
	import Stop from "carbon-icons-svelte/lib/Stop.svelte";
	import BrightnessContrast from "carbon-icons-svelte/lib/BrightnessContrast.svelte";
	//import Close from "carbon-icons-svelte/lib/Close.svelte";
	import Predictor from "./Predictor.svelte";
	import PredictorResults from "./PredictorResults.svelte";
	//import Maximize from "carbon-icons-svelte/lib/Maximize.svelte";
	//import Minimize from "carbon-icons-svelte/lib/Minimize.svelte";
	import { fly } from 'svelte/transition';
	import { pad2 } from "$lib/util";

	import * as THREE from 'three';
	import { onMount } from 'svelte';
	import ColorHash from 'color-hash';

	import workerURL from '$lib/components/globe.worker.js?url';

	import earthTexture from '$lib/assets/eo_base_2020_clean_3600x1800.png'//'$lib/assets/water_4k.png';
	//import { WordCloud } from 'carbon-icons-svelte';

	//let Globe;

	//const EARTH_RADIUS_KM = 6371; // km
	//const TIME_STEP = 1 * 1000; // per frame
	//const TIME_EVAL = 1000;

	//const colorHash = new ColorHash({ lightness: 0.5 });

	const COLORS = {
		1:  0x440154, // starlink
		2:  0xed6925, //'red', // oneweb
		10: 0x35b779, // pass
		11: 0xfde725  // pass-light
	};

	let root;
	let chart;
	let world;
	let worker;
	let UTCFullYear = 0;
    let UTCMonth = 0;
    let UTCDay = 0;
    let UTCHours = 0;
    let UTCMinutes = 0;
    let UTCSeconds = 0;
	let satTable = [];
	let satData = [];
	let satDataLastSize = 0;
	let threeGroupOfSats;

	let realistic = false;

	let timeMode = 'STOP';
	let isPreparingObserve = false;

	function resize(world) {
		world.width(window.innerWidth)
			.height(window.innerHeight);
	}

	const exec = (timeMode_) => {
		return _ => {
			if(worker) {
				worker.postMessage({
					type: 'setTimeMode',
					timeMode: timeMode_
				});
			}
			timeMode = timeMode_
		};
	}

	function toggleRealistic() {
		realistic = !realistic;
		if(world) {
			console.log('realistic', realistic)
			world
			.showAtmosphere(realistic)
			.showGraticules(!realistic)
			.globeImageUrl(
				realistic? "//unpkg.com/three-globe/example/img/earth-night.jpg" : earthTexture
			)
			.bumpImageUrl(
				realistic ? '//unpkg.com/three-globe/example/img/earth-topology.png' : null
			).backgroundImageUrl(
				realistic ? '//unpkg.com/three-globe/example/img/night-sky.png' : null
			)
		}
	}

	onMount(async () => {
		if(!browser) {
			return;
		}
		// import in this way because ssr
		//const Globe = await (await import('globe.gl')).default;
		const GlobeModule = await import("globe.gl");
		const Globe = GlobeModule.default;

		const OurGlobe = Globe({ animateIn: true });
		world = OurGlobe(chart)
			//.showAtmosphere(realistic)
			//.showGraticules(!realistic)
			.backgroundColor('#fff')
			.globeImageUrl(
				realistic? "//unpkg.com/three-globe/example/img/earth-night.jpg" : earthTexture
			)
			.objectLat('lat')
			.objectLng('lng')
			.objectAltitude('alt')
			.pointAltitude(0.7)
			.pointResolution(2)
			.pointRadius(0.05)
			.pointsMerge(true)
			.bumpImageUrl('//unpkg.com/three-globe/example/img/earth-topology.png');
		
			if(realistic) {
				world.globeImageUrl('//unpkg.com/three-globe/example/img/earth-night.jpg')
				.backgroundImageUrl('//unpkg.com/three-globe/example/img/night-sky.png')
			}

		world.enablePointerInteraction(false)

		var dotGeometry = new THREE.BufferGeometry();
		dotGeometry.setAttribute('position', new THREE.Float32BufferAttribute([0, 0, 0], 3));
		world.customThreeObject(item => {//world.customThreeObject(item => {
			const dotMaterial = new THREE.PointsMaterial({ size: 2.0, color: item.color || '#006bcc' });
			return new THREE.Points(dotGeometry, dotMaterial);
			let satPixel = threeGroupOfSats.getObjectByName(item.name);
			//console.log('color', item.color)
			if(!satPixel) {
				console.log('item.name!', item);
				return null;
			}
			satPixel.position.x = item.lng;
			satPixel.position.y = item.lat;
			satPixel.position.z = item.alt;
			satPixel.material.color.setHex(item.color);
			return satPixel;
		});
		/*world.customThreeObject(d => new THREE.Mesh(
        	new THREE.SphereBufferGeometry(.5),
        	new THREE.MeshLambertMaterial({ color: d.color })
      	));*/
		world.customThreeObjectUpdate((satPixel, item) => {
			//satPixel.position.x = item.lng;
			//satPixel.position.y = item.lat;
			//satPixel.position.z = item.alt;
			Object.assign(satPixel.position, world.getCoords(item.lat, item.lng, item.alt));
			satPixel.material.color.setHex(item.color);
		});

		window.addEventListener('resize', () => resize(world));
		resize(world)
		setTimeout(() => world.pointOfView({ lat: -24.08913333, lng: -69.93058889, altitude: 2.5 }));

		/*requestAnimationFrame(_ => {
			
		})*/
		/*const frame = _ => {
			world.customLayerData(satData); // objectsData
			requestAnimationFrame(frame);
		};*/
		//requestAnimationFrame(frame);

		//worker = new Worker(workerURL, { type: 'module' });
		const SyncWorker = await import('$lib/components/globe.worker.js?worker');
		worker = new SyncWorker.default();

		let repainting = false;
		worker.onmessage = (event) => {

			if (event.data instanceof ArrayBuffer) {
				//requestAnimationFrame(_ => {
					repainting = true;
					const data = new Float32Array(event.data);
					const size = (data.length - 6) / 4;
					let cursor = 0;
					UTCFullYear = data[cursor++]
					UTCMonth = data[cursor++]
					UTCDay = data[cursor++]
					UTCHours = data[cursor++]
					UTCMinutes = data[cursor++]
					UTCSeconds = data[cursor++]
					//satData.data = new Array(size);
					let i = 0,
						j = 0;
					let colorIndex;
					
					while (i++ < size) {
						colorIndex = data[cursor++];
						if (colorIndex < 0) {
							// error, so skyp sat data
							cursor += 3;
						} else {
							satData[j++] = {
								name: `sat${j}`,
								color: COLORS[colorIndex], //({1: 'blue', 2: 'red', 10: 'green', 11: 'yellow'})[colorIndex], // 1: Starlink, 2: OneWeb
								lat: data[cursor++],
								lng: data[cursor++],
								alt: data[cursor++],
							};
						}
					}
					if (i !== j) {
						// Here we remove the empty spaces carring from the error
						// (where the cursor is += 3)
						satData.splice(-(i - j), size);
					}
					if(satDataLastSize > size) {
						satData.splice(size, satDataLastSize);
					}
					satDataLastSize = size;
					//console.log(satData)
					world.customLayerData(satData);
				//});

			} else {
				const eventType = event.data.type;
				if (eventType === 'predictionFinished') {
					timeMode = 'PREDICTION_RESULTS';
					satTable = event.data.satTable
				} else if (eventType === 'tleLoaded') {

					const dotGeometry = new THREE.BufferGeometry();
					dotGeometry.setAttribute('position', new THREE.Float32BufferAttribute([0, 0, 0], 3));
					const dotMaterial = new THREE.PointsMaterial({ size: 2.0, color: 0x006bcc });
					const protoPixel = new THREE.Points(dotGeometry, dotMaterial);

					threeGroupOfSats = new THREE.Group();

					const size = event.data.size;
					for(let i = 0; i < size; i++) {
						const pixel = protoPixel.clone();
						pixel.name = `sat${i}` // TODO: Fix when some sats index is missing by error in propagator
						threeGroupOfSats.add(pixel);
					}

					worker.postMessage({ type: 'requestDataFrame' });
					
				}
			}
		};

	});

	
</script>

<div class="w-full h-full overflow-hidden" bind:this={root} id="a">
	<button on:click={toggleRealistic} class="fixed top-0 right-0 p-2 z-50">
		<BrightnessContrast size={32} />
	</button>
	<span class="fixed select-none cursor-help top-0 left-0 p-1 m-2 z-50 bg-gray-500 
	text-white font-medium text-xs rounded-lg"
	title="This version is a build of the research development version, just to test if the concept works.
	This project was proposed originally to CLEOSat research group.
	'Sun Industries' is a company I intend to found">
	? | PRE-ALPHA VERSION
	</span>
	<div bind:this={chart} />
	<div class="fixed bottom-12 left-4 text-xs leading-none">
		<div class="flex flex-col">
			<div class="flex flex-row items-center p-1">
				<div class="bg-[blue] w-[4px] h-[4px] mr-2"></div>
				<span>Starlink</span>
			</div>
			<div class="flex flex-row items-center p-1">
				<div class="bg-[red] w-[4px] h-[4px] mr-2"></div>
				<span>OneWeb</span>
			</div>
			<div class="flex flex-row items-center p-1">
				<div class="bg-[green] w-[4px] h-[4px] mr-2"></div>
				<span>Observed/No Sunlight</span>
			</div>
			<div class="flex flex-row items-center p-1">
				<div class="bg-[yellow] w-[4px] h-[4px] mr-2"></div>
				<span>Observed/Sunlight</span>
			</div>
		</div>
	</div>
	<div transition:fly="{{ y: 200, duration: 250 }}"
	class="fixed m-0 w-full left-0 right-0 h-11 bottom-0 bg-black z-50">
		<div class="flex flex-1 flex-row justify-between  h-full">
			<div class="w-24"></div>
			{#if timeMode !== 'PREDICTION'}
			<div class="flex flex-row p-1 text-white">
				<button on:click={exec('REWIND')}>
					<svg class="rotate-180" xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" aria-hidden="true" role="img" width="32" height="32" preserveAspectRatio="xMidYMid meet" viewBox="0 0 32 32"><path fill="currentColor" d="M15 5v7.375L6.594 6.187L5 5.032V26.97l1.594-1.157L15 19.625V27l1.594-1.188l12-9L29.656 16l-1.062-.813l-12-9zM7 8.969l8.406 6.187l1.125.844l-1.125.844L7 23.03zM17 9l9.313 7L17 23z"></path></svg>
				</button>
				<button on:click={exec('STOP')}>
					<Stop size={32} />
				</button>
				{#if timeMode === 'PLAY'}
				<button on:click={exec('PAUSE')}>
					<Pause size={32} />
				</button>
				{:else}
				<button on:click={exec('PLAY')}>
					<Play size={32} />
				</button>
				{/if}
				<button on:click={exec('FORWARD')}>
					<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" aria-hidden="true" role="img" width="32" height="32" preserveAspectRatio="xMidYMid meet" viewBox="0 0 32 32"><path fill="currentColor" d="M15 5v7.375L6.594 6.187L5 5.032V26.97l1.594-1.157L15 19.625V27l1.594-1.188l12-9L29.656 16l-1.062-.813l-12-9zM7 8.969l8.406 6.187l1.125.844l-1.125.844L7 23.03zM17 9l9.313 7L17 23z"></path></svg>
				</button>
			</div>
			{/if}
			<button class="flex flex-col text-xs w-24 pl-2 pr-2 items-center p-1 text-white">
				<span class="flex flex-row text-center items-baseline">
					<span style="width: 14.5px;">{pad2(UTCHours)}</span>:
					<span style="width: 14.5px;">{pad2(UTCMinutes)}</span>:
					<span style="width: 14.5px;">{pad2(UTCSeconds)}</span>
					<span style="font-size: 8px;">&#32;UTC</span>
				</span>
				<span>{UTCFullYear}/{pad2(UTCMonth + 1)}/{pad2(UTCDay)}</span>
			</button>
		</div>
	</div>
	<Predictor {worker} bind:isPreparingObserve={isPreparingObserve} bind:timeMode={timeMode} />
	<PredictorResults {worker} {satTable} bind:timeMode={timeMode} />
</div>
