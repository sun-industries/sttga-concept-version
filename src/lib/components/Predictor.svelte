<script>
	import { fly } from 'svelte/transition';
	import * as satellite from 'satellite.js';
	import IconWindowMin from '$lib/assets/icons/WindowMin.svelte'
	import IconWindowMax from '$lib/assets/icons/WindowMax.svelte'
	import IconWindowRestore from '$lib/assets/icons/WindowRestore.svelte'
	import Frame from './Frame.svelte';


	const observatories = [
        {
			name: '*New',
			lat: 0, // degrees
			lng: 0, // degrees
			alt: 0, // km
			elv: 20 // degrees (90 is zenith)
		},
		{
			name: 'Ckoirama',
			lat: -24.08913333, // degrees
			lng: -69.93058889, // degrees
			alt: 0.966, // km
			elv: 20 // degrees (90 is zenith)
		},
		{
			name: 'ESO, Paranal',
			lat: -24.6275,
			lng: -70.4044,
			alt: 2.65,
			elv: 20
		},
		{
			name: 'Mauna Kea',
			lat: 19.82222,
			lng: -155.47494,
			alt: 4.207,
			elv: 20
		},
		{
			name: 'Canarias',
			lat: 28.756611,
			lng: -17.892028,
			alt: 2.3,
			elv: 20
		},
		{
			name: 'SAAO',
			lat: -32.379444,
			lng: 20.810556,
			alt: 1.798,
			elv: 20
		},
		{
			name: 'Tianyan',
			lat: 25.65289,
			lng: 106.85678,
			alt: 0,
			elv: 20
		}
	];

	let selectedObservatoryName = 'Ckoirama';
	let observatory;
	let observerGd;
	let satelliteLog;

    let selectedObs_name = '';
    let selectedObs_lat = '';
    let selectedObs_lng = '';
    let selectedObs_alt = '';
    let selectedObs_elv = '';
    let setting_date = '';
    let setting_mode = '';

    export let worker;
    export let timeMode;
    //export let isPreparingObserve;

	let status = 'MINIMIZED';

	function onSelectObservatory() {
		satelliteLog = [];
		if (selectedObservatoryName) {
			observatory = Object.entries(observatories).find(
				([_, { name }]) => name === selectedObservatoryName
			)[1];
			observerGd = observerGround(observatory);
            selectedObs_name = observatory.name;
            selectedObs_lat = observatory.lat;
            selectedObs_lng = observatory.lng;
            selectedObs_alt = observatory.alt;
            selectedObs_elv = observatory.elv;
		} else {
			observatory = false;
			observerGd = false;
		}
	}

	const observerGround = (observatory) => ({
		longitude: satellite.degreesToRadians(observatory.lng),
		latitude: satellite.degreesToRadians(observatory.lat),
		height: observatory.height,
		minElevation: observatory.minElevation
	});

    function onStart() {
        const data = {
            name: selectedObs_name,
            from: {
                latitude: satellite.degreesToRadians(selectedObs_lat),
                longitude: satellite.degreesToRadians(selectedObs_lng),
                height: selectedObs_alt,
                minElevation: selectedObs_elv
            },
            date: setting_date,
            mode: setting_mode
        }
        worker.postMessage({
			type: 'startPrediction',
			...data
		});
        timeMode = 'PREDICTION';
        status = 'CLOSED'
    }

    onSelectObservatory();
</script>

<!--{#if isPreparingObserve}-->
<!--<div transition:fly="{{ y: 200, duration: 250 }}"
	class="fixed 
    left-0 right-0 bottom-0 h-96
    backdrop-blur-sm bg-white/90
    border-2
    p-4"
>-->
<Frame closable={false} bind:status title="Predictor" label="Predict">
	<fieldset class="w-full grid gap-4 grid-flow-row grid-cols-2 border-l pl-4">
		<div class="flex flex-col flex-1">
			<label for="obs-selector" class="text-xs leading-none">Observatory</label>
			<select id="obs-selector"
            class="bg-transparent outline-none border-b-2"
            bind:value={selectedObservatoryName}
            on:change={onSelectObservatory}>
				{#each observatories as obs}
					<option value={obs.name}>{obs.name}</option>
				{/each}
			</select>
		</div>
		<div class="flex flex-col flex-1">
			<label for="obs-name" class="text-xs leading-none">Name</label>
			<input type="text" id="obs-name"
            class="bg-transparent outline-none border-b-2"
            bind:value={selectedObs_name} />
		</div>
		<div class="flex flex-col flex-1">
			<label for="obs-lat" class="text-xs leading-none">Latitude</label>
			<input type="number" id="obs-lat"
            class="bg-transparent outline-none border-b-2"
            bind:value={selectedObs_lat} placeholder="Degree" />
		</div>
		<div class="flex flex-col flex-1">
			<label for="obs-lng" class="text-xs leading-none">Longitude</label>
			<input id="obs-lng"
            class="bg-transparent outline-none border-b-2"
            bind:value={selectedObs_lng} placeholder="Degree" />
		</div>
		<div class="flex flex-col flex-1">
			<label for="obs-alt" class="text-xs leading-none">Altitude</label>
			<input id="obs-alt" 
            class="bg-transparent outline-none border-b-2"
            bind:value={selectedObs_alt} placeholder="km over the sea" />
		</div>
		<div class="flex flex-col flex-1">
			<label for="obs-elv" class="text-xs leading-none">Min Elevation</label>
			<input id="obs-elv" 
            class="bg-transparent outline-none border-b-2"
            bind:value={selectedObs_elv} placeholder="Degree" />
		</div>
	</fieldset>
	<fieldset class="w-full grid gap-4 grid-flow-row grid-cols-2 border-l pl-4 pt-4 pb-4 mt-2 mb-2">
		<div class="flex flex-col flex-1">
			<label for="obs-mode">When</label>
			<input class="bg-transparent outline-none border-b-2" type="date"
            bind:value={setting_date}
             />
		</div>
		<div class="flex flex-col flex-1">
			<label for="obs-mode">At</label>
			<select id="obs-mode" class="bg-transparent outline-none border-b-2"
            bind:value={setting_mode}
            >
				<option>All day</option>
				<option>Morning</option>
				<option>Evening</option>
			</select>
		</div>
	</fieldset>
    <div class="text-center">
        <button on:click={onStart} class="m-4 font-semibold bg-light-800">START</button>
    </div>
    <pre>

    </pre>
	<table class="w-full hidden">
		<tr>
			<th>Time</th>
			<th>Time</th>
			<th>Time</th>
			<th>Time</th>
		</tr>
		<tr>
			<td>sdf</td>
			<td>sdf</td>
			<td>sdf</td>
			<td>sdf</td>
		</tr>
	</table>
</Frame>
<!--{/if} -->