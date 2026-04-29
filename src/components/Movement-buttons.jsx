export const MovButton = ({ children, onClick, className }) => (
  <button 
    onClick={onClick}
    className={`flex items-center justify-center border-[1.5px] rounded-lg p-2 text-[14px] tracking-[4.2px] ${className} `}
  >
    {children}
  </button>
);