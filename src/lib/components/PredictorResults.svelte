<script>
import Frame from './Frame.svelte';
import { date2time, ra2dms } from '$lib/util'

    //export let worker;
    export let timeMode;
    export let satTable = [];

    let fromTime;
    let toTime;
    let progress = 0;
    let spreadsheetDom;
    let status = 'CLOSED';
    let selectedSatIndex;
    $: selectedSat = satTable[selectedSatIndex];
    $: {
        if(timeMode == 'PREDICTION_RESULTS') {
            status = 'RESTORED'
        }
    }

</script>

<Frame closable={true} bind:status title="Predictor Results" label="Results">
    <div class="h-full flex flex-col lg:flex-row overflow-hidden">
        <div class="h-full flex-1 overflow-auto">
            <select>
                <option>Sort by Name</option>
                <option>Sort by Time</option>
            </select>
            <table class="prose text-xs">
                <tr class="border-b sticky top-0 backdrop-blur-sm bg-white/80">
                    <th class="text-center">Starting</th>
                    <th class="text-center">Ending</th>
                    <th class="text-center">Satellite</th>
                    <th class="text-center">Transit</th>
                    <th class="text-center">Max Elevation</th>
                    <th class="text-center">Apex Azimuth</th>
                    <th class="text-center">Sunlight</th>
                    <th class="text-center"></th>
                </tr>
                {#each satTable as item, index }
                <tr class="hover:bg-[#192B64] hover:bg-opacity-5">
                    <td class="text-center">{date2time(item.starting)}</td>
                    <td class="text-center">{date2time(item.ending)}</td>
                    <td class="text-left">{item.satName}</td>
                    <td class="text-center">{item.transit}</td>
                    <td class="text-center">{item.maxElevation}</td>
                    <td class="text-center">{item.apexAximuth}</td>
                    <td class="text-center">{(item.sunlitRatio * 100).toFixed(2)}%</td>
                    <td>
                        <button class="m-1 p-1 pr-2 pl-2 text-[#192B64]"
                        on:click={_ => selectedSatIndex = index}
                        >Open</button>
                    </td>
                </tr>
                {/each}
            </table>
        </div>
        {#if selectedSat}
        <div class="h-full flex-1 overflow-auto">
            <table class="prose text-xs">
                <tr class="border-b sticky top-0 backdrop-blur-sm bg-white/80">
                    <th>Time</th>
                    <th>Azimuth</th>
                    <th>Elevation</th>
                    <th>Range (km)</th>
                    <th>Sunlight</th>
                </tr>
                {#each selectedSat.detailed.time as time, index}
                <tr class="hover:bg-[#192B64] hover:bg-opacity-5">
                    <td class="text-center">{date2time(time)}</td>
                    <td class="text-center">{ra2dms(selectedSat.detailed.azimuth[index])}</td>
                    <td class="text-center">{ra2dms(selectedSat.detailed.elevation[index])}</td>
                    <td class="text-right">{selectedSat.detailed.rangeSat[index]}</td>
                    {#if selectedSat.detailed.sunlit[index]}
                    <td class="text-center">Yes</td>
                    {:else}
                    <td></td>
                    {/if}
                </tr>
                {/each}
            </table>
        </div>
        {/if}
    </div>
</Frame>

<div class="fixed h-1 bottom-0 left-0 right-0">
    <div class="bg-blue-400 h-full" style="width: {0}%;"></div>
</div>