export const SpeedSlider = ({value, onChange, className}) => (
    <input
        type='range'
        min='0'
        max='100'
        step='1'
        value={value}
        onChange={(e) => onChange(Number(e.target.value))}
        className={`mt-3 w-full h-3.25 ${className} rounded-[20px] appearance-none [&::-webkit-slider-thumb]:appearance-none [&::-webkit-slider-thumb]:w-6 [&::-webkit-slider-thumb]:h-5.75 [&::-webkit-slider-thumb]:rounded-full [&::-webkit-slider-thumb]:bg-[#3CE89B] [&::-webkit-slider-thumb]:shadow-[0_0_5px_#3CE89B] [&::-webkit-slider-thumb]:cursor-pointer [&::-webkit-slider-thumb]:bg-linear-to-r [&::-webkit-slider-thumb]:from-[#3CE89B] [&::-webkit-slider-thumb]:to-[#228257] [&::-webkit-slider-thumb]:border [&::-webkit-slider-thumb]:border-[#2C4A3A] `}
    />
);