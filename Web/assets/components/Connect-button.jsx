export const ConnectButton = ({ children, onClick }) => (
    <button 
        onClick = {onClick}
        className='flex justify-center items-center h-9.5 w-39 gap-1.75 bg-[#0F1729] px-4.5 py-1.5 cursor-pointer border border-[#33CCCC] uppercase rounded-lg tracking-[1px] text-[11px] text-[#33CCCC] shadow-[0_0_#33CCCC80,0_0_5px_#33CCCC40] ' 
    >
        {children}
    </button>
);