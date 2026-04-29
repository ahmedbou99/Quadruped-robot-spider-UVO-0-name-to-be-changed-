export const RotateNavButton = ({ children, onClick, className }) => (
  <button 
    onClick={onClick}
    className={`flex items-center justify-center border-[1.5px] rounded-lg w-12.5 h-12.5 ${className}`}
  >
    {children}
  </button>
);