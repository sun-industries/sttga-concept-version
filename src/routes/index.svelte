<script>
	import Globe from '$lib/components/Globe.svelte';

	import logo10 from '$lib/assets/sun-web.svg';

</script>

<div class="w-full h-full m-0 overflow-hidden">
	<div class="fixed bottom-11 right-0 p-1 pl-4 text-blue-gray-900 bg-white bg-opacity-95 z-50">
		<div class="flex flex-row">
			<div class="pr-2 text-center flex items-center">
				<a href="https://sun.industries">
				<img src={logo10} class="h-4 m-auto lg:m-0" alt="Hypergea Logo" />
				</a>
			</div>
			<div class="w-32 pl-2 border-l border-solid border-blue-gray-900 text-xs leading-none
			flex items-center">
				<small>Satellite Tracking Tool for Ground Astronomy</small>
			</div>
		</div>
	</div>

	<Globe />

	
</div>
