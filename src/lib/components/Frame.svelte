<script>
    import { fly } from 'svelte/transition';
	import IconWindowMin from '$lib/assets/icons/WindowMin.svelte'
	import IconWindowMax from '$lib/assets/icons/WindowMax.svelte'
	import IconWindowRestore from '$lib/assets/icons/WindowRestore.svelte'
    import IconWindowClose from '$lib/assets/icons/WindowClose.svelte'

    export let status = 'MINIMIZED';
    export let closable = true;
    export let title = ''
    export let label = 'App'
    export let padding = true;

</script>

{#if status != 'MINIMIZED' && status != 'CLOSED'}
<div transition:fly="{{ y: 200, duration: 250 }}"
	class="fixed flex flex-col
    left-0 right-0 bottom-11 {status === 'MAXIMIZED' ? 'top-0' : 'h-96'}
    backdrop-blur-sm bg-white/80
    p-0"
    _style="{status === 'MAXIMIZED' ? 'height:calc(100vh - 44px)!important' : ''}"
>
	<div class="w-full flex flex-row">
		<h1 class="flex-1 font-extrabold text-left pl-4">{title}</h1>
		<div class="flex flex-row">
			<button class="w-8 h-8" on:click={_ => status = 'MINIMIZED'}>
				<IconWindowMin class="w-8 h-8" />
			</button>
            {#if status !== 'MAXIMIZED'}
			<button class="w-8 h-8" on:click={_ => status = 'MAXIMIZED'}>
				<IconWindowMax class="w-8 h-8" />
			</button>
            {/if}
            {#if status !== 'RESTORED'}
            <button class="w-8 h-8" on:click={_ => status = 'RESTORED'}>
                <IconWindowRestore class="w-8 h-8" />
            </button>
            {/if}
            <button class="w-8 h-8 disabled:text-gray-400" disabled={!closable} on:click={_ => status = 'CLOSED'}>
				<IconWindowClose class="w-8 h-8" />
			</button>
		</div>
	</div>
    <div class="h-full flex-1 overflow-auto">
        {#if padding}
        <div class="h-full p-2"><slot /></div>
        {:else}
        <slot />
        {/if}
    </div>
</div>
{/if}
{#if status !== 'CLOSED'}
<button on:click={_ => {status = status == 'MINIMIZED' ? 'RESTORED' : 'MINIMIZED'; console.log('status', status)}}
    class="fixed z-50 bottom-3 border border-transparent pl-2 pr-2 flex flex-row items-center  font-bold
    {status !== 'MINIMIZED' ? 'border-t-white border-l-current border-r-current border-b-current bg-white text-[#192B64]':'text-white'}">
        <!--<Icon icon={telescopeIcon} width="24" height="24" />-->
        <span class="pl-2 pr-2">{label}</span>
</button>
{/if}