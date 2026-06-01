import { useState, useEffect,useRef } from 'react'
import mqtt from 'mqtt'
import './index.css'
import { ArrowNavButton } from './components/Arrow-Nav-buttons';
import { ArrowUp, ArrowDown, RotateCcw, RotateCw, Camera } from 'lucide-react';
import { MovButton } from './components/Movement-buttons';
import { RotateNavButton } from './components/Rotate-Nav-Button';
import { SpeedSlider } from './components/Speed_Slider';

// MQTT CONFIG
const BROKER_URL  = 'wss://mqtt.local:9001';        // WebSocket TLS 
const MQTT_OPTS = {
  username : 'dashboard',                            // dashboard user 
  password : 'dash123',
  clientId : `dashboard_${Math.random().toString(16).slice(2,8)}`, // unique client ID
  clean    : true,
};

// TOPICS
const TOPICS = {
  CMD_DIRECTION : 'robot/spider/cmd/direction',
  CMD_SPEED     : 'robot/spider/cmd/speed',
  CMD_GAIT      : 'robot/spider/cmd/gait',
  CMD_ROTATION  : 'robot/spider/cmd/rotation',
  CAM_STREAM    : 'robot/spider/cam/stream',
  STATUS        : 'robot/spider/status',
};


const QOS_CMD    = 1;   // movement commands
const QOS_STREAM = 0;   // camera stream
const QOS_STATUS = 1;   // LWT / status




// Broker Connection Modal 
const BrokerModal = ({ onCancel, onConnect }) => {
  const [form, setForm] = useState({ host: 'mqtt.local', port: '9001', clientId: 'dashboard', password: 'dash123' });
  const [errors, setErrors] = useState({ host: '', port: '', clientId: '', password: '' });
  const [authError, setAuthError] = useState('');

  const set = (field) => (e) => {
    setForm(f => ({ ...f, [field]: e.target.value }));
    setErrors(err => ({ ...err, [field]: '' }));
    setAuthError('');
  };

  const handleConnect = () => {
    const newErrors = { host: '', port: '', clientId: '', password: '' };
    let valid = true;
    if (!form.host.trim())   { newErrors.host     = 'Broker IP / Host is required.'; valid = false; }
    if (!form.port.trim())   { newErrors.port     = 'Port is required.';             valid = false; }
    else if (!/^\d+$/.test(form.port.trim())) { newErrors.port = 'Port must be a number.'; valid = false; }
    if (!form.clientId.trim()) { newErrors.clientId = 'Username is required.';       valid = false; }
    if (!form.password.trim()) { newErrors.password = 'Password is required. Leave blank only if the broker is anonymous.'; valid = false; }
    setErrors(newErrors);
    if (!valid) return;
    onConnect(form, setAuthError);
  };

  const handleKeyDown = (e) => {
    if (e.key === 'Enter') {
      e.preventDefault();
      handleConnect();
    }
  };

  const inputCls = (field) =>
    `w-full bg-[#324052]/50 rounded-md px-3 py-2 text-[#C8D3DF] text-[14px] tracking-wide outline-none transition-colors placeholder:text-[#A3B9C2] ${
      errors[field]
        ? 'border border-[#DB2424] focus:border-[#DB2424]'
        : 'focus:border-[#33CCCC]'
    }`;

  const labelCls = 'text-[#627084] text-[9px] uppercase tracking-[3px] mb-1';

  const ErrorMsg = ({ field }) =>
    errors[field] ? <p className='text-[#DB2424] text-[9px] mt-1 tracking-wide'>{errors[field]}</p> : null;

  return (
    <div 
      className='fixed inset-0 z-50 flex items-center justify-center bg-black/5 backdrop-blur-xs' 
      onClick={onCancel}
      onKeyDown={handleKeyDown}
      >
      <div className='w-90 bg-[#0f1C2D] border-[1.5px] border-[#33CCCC]/60 rounded-xl p-6 flex flex-col gap-4 shadow-[0_0_15px_#33CCCC70] font-["Orbitron"]' onClick={e => e.stopPropagation()}>
        <div className='text-[#33CCCC] font-bold text-[14px] uppercase tracking-[4px]'>Broker Connection</div>

        {authError && (
          <div className='bg-[#DB2424]/10 border border-[#DB2424]/60 rounded-lg px-3 py-2 text-[#DB2424] text-[10px] tracking-wide'>
            {authError}
          </div>
        )}

        <div className='flex flex-col gap-3'>
          <div><div className={labelCls}>Broker IP / Host</div><input className={inputCls('host')} value={form.host} onChange={set('host')} placeholder='localhost' /><ErrorMsg field='host' /></div>
          <div><div className={labelCls}>Port (Websocket)</div><input className={inputCls('port')} value={form.port} onChange={set('port')} placeholder='9801' /><ErrorMsg field='port' /></div>
          <div><div className={labelCls}>Username</div><input className={inputCls('clientId')} value={form.clientId} onChange={set('clientId')} placeholder='dashboard' /><ErrorMsg field='clientId' /></div>
          <div><div className={labelCls}>Password</div><input className={inputCls('password')} type='password' value={form.password} onChange={set('password')} placeholder='leave empty if anonymous' /><ErrorMsg field='password' /></div>
        </div>

        <div className='flex gap-3 pt-1'>
          <button onClick={onCancel} className='flex-1 h-10 bg-transparent border border-[#C8D3DF]/40 rounded-lg text-[#C8D3DF] text-[11px] uppercase tracking-[2px] hover:border-[#C8D3DF] transition-colors cursor-pointer'>Cancel</button>
          <button onClick={handleConnect} className='flex-1 h-10 bg-transparent border border-[#33CCCC] rounded-lg text-[#33CCCC] text-[11px] uppercase tracking-[2px] hover:bg-[#33CCCC]/10 transition-colors shadow-[0_0_8px_#33CCCC40] cursor-pointer font-semibold'>Connect</button>
        </div>
      </div>
    </div>
  );
};

// Disconnect Confirmation Modal 
const DisconnectModal = ({ onCancel, onConfirm }) => (
  <div className='fixed inset-0 z-50 flex items-center justify-center bg-black/5 backdrop-blur-xs' onClick={onCancel}>
    <div className='w-90 bg-[#0f1729] border-[1.5px] border-[#DB2424]/60 rounded-xl p-6 flex flex-col gap-5 shadow-[0_0_15px_#DB242470] font-["Orbitron"]' onClick={e => e.stopPropagation()}>
      <div className='text-[#DB2424] font-bold text-[14px] uppercase tracking-[4px]'>Disconnect</div>
      <p className='text-[#C8D3DF] text-[11px] leading-relaxed tracking-[1px]'>Are you sure you want to disconnect from this broker?</p>
      <div className='flex gap-3'>
        <button onClick={onCancel} className='flex-1 h-10 bg-transparent border border-[#C8D3DF]/40 rounded-lg text-[#C8D3DF] text-[11px] uppercase tracking-[2px] hover:border-[#C8D3DF] transition-colors cursor-pointer'>Cancel</button>
        <button onClick={onConfirm} className='flex-1 h-10 bg-transparent border border-[#DB2424] rounded-lg text-[#DB2424] text-[11px] uppercase tracking-[2px] shadow-[0_0_8px_#DB242440] hover:bg-[#DB2424]/10 transition-colors cursor-pointer font-semibold'>Disconnect</button>
      </div>
    </div>
  </div>
);



// Main App
function App() {
  const [activeKey, setActiveKey]                   = useState(null);
  const [showConnectModal, setShowConnectModal]     = useState(false);
  const [showDisconnectModal, setShowDisconnectModal] = useState(false);
  const [connected, setConnected]                   = useState(false);
  const [camFrame, setCamFrame]                       = useState(null);  // camera frame state
  // const [robotOnline, setRobotOnline]                 = useState(false); //  LWT status
  const [speed, setSpeed]                             = useState(50);    // speed state for publish
  const [activeGait, setActiveGait]                   = useState(null);  // gait state
  const mqttClientRef                                 = useRef(null);    // holds the mqtt.js client

  // Keyboard events
  useEffect(() => {
    const handleKeyDown = (e) => {
      if (['ArrowUp', 'ArrowDown', 'ArrowLeft', 'ArrowRight'].includes(e.key)) e.preventDefault();
      setActiveKey(e.key);
    };
    const handleKeyUp = () => setActiveKey(null);
    window.addEventListener('keydown', handleKeyDown);
    window.addEventListener('keyup', handleKeyUp);
    return () => {
      window.removeEventListener('keydown', handleKeyDown);
      window.removeEventListener('keyup', handleKeyUp);
    };
  }, []);

  // Publish direction on arrow key press 
  useEffect(() => {
    if (!connected || !mqttClientRef.current) return;
    const dirMap = {
      ArrowUp    : 'forward',
      ArrowDown  : 'backward',
      ArrowLeft  : 'left',
      ArrowRight : 'right',
    };
    const dir = dirMap[activeKey];
    if (dir) {
      publish(TOPICS.CMD_DIRECTION, { direction: dir }, QOS_CMD);
    } else if (activeKey === null) {
      // key released → send stop
      publish(TOPICS.CMD_DIRECTION, { direction: 'stop' }, QOS_CMD);
    }
  }, [activeKey, connected]);

  // MQTT helpers 
  const publish = (topic, payload, qos = QOS_CMD) => {
    if (!mqttClientRef.current) return;
    mqttClientRef.current.publish(topic, JSON.stringify(payload), { qos });
  };

  const handleHeaderButton = () => {
    if (connected) setShowDisconnectModal(true);
    else           setShowConnectModal(true);
  };

  const handleModalConnect = (form, setAuthError) => {
    const url  = `wss://${form.host}:${form.port}`;
    const opts = {
      ...MQTT_OPTS,
      username : form.clientId,
      password : form.password,
    };

    const client = mqtt.connect(url, opts);

    client.on('connect', () => {
      client.subscribe(TOPICS.CAM_STREAM, { qos: QOS_STREAM });
      client.subscribe(TOPICS.STATUS,     { qos: QOS_STATUS });
      mqttClientRef.current = client;
      setShowConnectModal(false);
      setConnected(true);
    });

    // Handle incoming messages from broker
    client.on('message', (topic, message) => {
      try {
        const payload = JSON.parse(message.toString());

        if (topic === TOPICS.CAM_STREAM) {
          // frame is base64 JPEG
          setCamFrame(`data:image/jpeg;base64,${payload.frame}`);
        }

        if (topic === TOPICS.STATUS) {
          // LWT — "online" | "offline"
          //setRobotOnline(payload.status === 'online');
        }
      } catch (err) {
        console.error('MQTT parse error:', err);
      }
    });

    // Auth / connection error handling
    client.on('error', (err) => {
      console.error('MQTT error:', err);
      if (err.message?.includes('Not authorized') || err.message?.includes('Bad username')) {
        setAuthError('Incorrect credentials. Please check username and password.');
      } else {
        setAuthError(`Connection failed: ${err.message}`);
      }
      client.end(true);
    });
  };

  const handleConfirmDisconnect = () => {
    if (mqttClientRef.current) {
      mqttClientRef.current.end();
      mqttClientRef.current = null;
    }
    setShowDisconnectModal(false);
    setConnected(false);
    setCamFrame(null);
    //setRobotOnline(false);
  };

  // Gait publish 
  const GAIT_MAP = {
    WALK   : 'tripod',
    WIGGLE : 'wave',
    CRAWL  : 'crawl',
    STAND  : 'ripple',
  };

  const handleGait = (label) => {
    if (!connected) return;
    const gait = GAIT_MAP[label];
    setActiveGait(label);
    publish(TOPICS.CMD_GAIT, { gait }, QOS_CMD);
    setTimeout(() => setActiveGait(null), 150);
  };

  // Speed publish 
  const handleSpeed = (val) => {
    setSpeed(val);
    if (!connected) return;
    publish(TOPICS.CMD_SPEED, { speed: val }, QOS_CMD);
  };

  // Rotation publish (rotate buttons)
  const handleRotate = (angle) => {
    if (!connected) return;
    publish(TOPICS.CMD_ROTATION, { angle }, QOS_CMD);
  };

  const isActive = (key) => activeKey === key;

  const panelCls = connected
    ? 'border border-[#33CCCC] shadow-[0_0_8px_#33CCCC20]'   // connected
    : 'border border-[#17CFBF]/40 ';                           // disconnected

  const titleCls = connected ? 'text-[#33CCCC]' : 'text-[#33CCCC]';
  const iconCls = connected ? 'stroke-[#0F1C2D] group-hover:stroke-[#0F1729]' : 'stroke-[#627084]';

  return (
    <>
      <div className='h-full bg-[#0f1729] font-["Orbitron"] overflow-hidden'>

        {/* Modals */}
        {showConnectModal    && <BrokerModal     onCancel={() => setShowConnectModal(false)}    onConnect={handleModalConnect}      />}
        {showDisconnectModal && <DisconnectModal onCancel={() => setShowDisconnectModal(false)} onConfirm={handleConfirmDisconnect} />}

        {/* Header */}
        <div className='flex justify-between items-center px-6 py-3'>
          <div className='flex items-center gap-3'>

            {/* Robot icon box*/}
            <div className={`flex items-center justify-center h-10 w-10 bg-[#1b282f] rounded-md border-[1.5px] border-[#0284C5]/30 ${connected ? 'shadow-[0_0_10px_#2BA77080,0_0_20px_#2BA77040]' : 'shadow-[0_0_10px_#E5282880,0_0_20px_#E5282840]'}`}>
              <img src={connected ? '/Robot_icon_green.png' : '/Robot_icon_red.png'} className='w-5 h-5 object-contain' alt='robot' />
            </div>

            {/* QUADBOT title */}
            <div>
              <div className={`text-[16px]  ${connected ? 'text-[#0284C5]' : 'text-[#53666E]'  } tracking-widest font-["Orbitron"]`}>
                QUAD<span className={connected ? 'text-[#2BA770]' : 'text-[#DB2424]'}>BOT</span>
              </div>

            { /* Connected/Disconnected Status */ }
              <div className={`flex justify-between items-center h-4.25 ${connected ? 'w-16' : 'w-20' } pt-0.75 pr-1.25 pl-2 pb-1 border-[0.5px] rounded-[30px] ${connected ? 'text-[#2BA770] border-[#003606] ' : 'text-[#AE070A] border-[#AE070A] '} `} >
                <div className='text-[8px] font-space'>
                  {connected ? 'connected' : 'disconnected'}
                </div>
                <span className={`w-1 h-1 rounded-full ${connected ? 'bg-[#2BA770]' : 'bg-[#AE070A]'}`}></span> 
              </div>

              {/* robot online/offline dot from LWT status 
              {connected && (
                <div className={`text-[9px] uppercase tracking-[2px] flex gap-1.5 ${robotOnline ? 'text-[#2BA770]' : 'text-[#DB2424]'}`}>
                  <span className={`w-1 h-1 rounded-full ${robotOnline ? 'bg-[#2BA770]' : 'bg-[#DB2424]'}`}></span>
                  {robotOnline ? 'Robot Online' : 'Robot Offline'}
                </div>
              )} */}

            </div>
          </div>

          {/* Connect/Disconnect button */}
          <button
            onClick={handleHeaderButton}
            className={`flex justify-center items-center h-9.5 px-5 py-1.5 cursor-pointer border rounded-lg uppercase tracking-[1px] text-[11px] transition-all ${
              connected
                ? 'bg-[#0F1729] border-[#DB2424] text-[#DB2424] shadow-[0_0_8px_#DB242440] hover:bg-[#DB2424]/10'
                : 'bg-[#0F1729] border-[#33CCCC] text-[#33CCCC] shadow-[0_0_8px_#33CCCC40] hover:bg-[#33CCCC]/10'
            }`}
          >
            {connected ? 'Disconnect' : 'Connect'}
          </button>
        </div>

        {/* ── Main grid ── */}
        <div className='grid grid-cols-2 gap-4 px-6 pt-3 pb-4 h-[calc(100vh-65px)]'>

          {/* LEFT COLUMN */}
          <div className='flex flex-col gap-3 min-w-0'>

            {/* Movement Control panel */}
            <div className={`flex flex-col  h-54.5 rounded-lg p-4 ${panelCls}`}>
              <div className={`text-[14px] tracking-[4px] ${titleCls}`}>Movement Control</div>

              <div className='flex justify-center items-center grow shrink'>
                <div className='grid grid-cols-[50px_50px_50px] grid-rows-[50px_50px_50px] gap-1.5'>

                  {/* Row 1 — Up */}
                  <div />
                  <ArrowNavButton className={`transition-all duration-75 ${isActive('ArrowUp') ? 'scale-95 bg-[#3CE89B] shadow-[0_0_9.6px_#3CE89B40,0_0_5px_#3CE89B40] bg-linear-to-r from-[#3CE89B] to-[#228257] transition-all' : ''} ${connected ? 'border-[#C8D3DF] bg-[#324052]/50 cursor-pointer hover:bg-[#3CE89B] hover:border-0 hover:shadow-[0_0_9.6px_#3CE89B40,0_0_5px_#3CE89B40] hover:bg-linear-to-r hover:from-[#3CE89B] hover:to-[#228257] transition-all' : 'border-[#2A3646] bg-[#0F1C2D]'} `}>
                    <ArrowUp className={`w-4.5 h-4.5 stroke-2 transition-colors ${isActive('ArrowUp') ? 'stroke-[#0F1729]' : iconCls} `} />
                  </ArrowNavButton>
                  <div />

                  {/* Row 2 — Rotate left / center dot / Rotate right */}
                  <RotateNavButton
                    onClick={() => handleRotate(0)}
                    className={`transition-all duration-75 ${isActive('ArrowLeft') ? 'scale-95 bg-[#3CE89B] shadow-[0_0_9.6px_#3CE89B40,0_0_5px_#3CE89B40] bg-linear-to-r from-[#3CE89B] to-[#228257]' : ''} ${connected ? 'border-[#C8D3DF] bg-[#324052]/50 cursor-pointer hover:bg-[#3CE89B] hover:border-0 hover:shadow-[0_0_9.6px_#3CE89B40,0_0_5px_#3CE89B40] hover:bg-linear-to-r hover:from-[#3CE89B] hover:to-[#228257] transition-all' : 'border-[#2A3646] bg-[#0F1C2D]'}`}>
                    <RotateCcw className={`w-4.5 h-4.5 stroke-2 transition-colors ${isActive('ArrowLeft') ? 'stroke-[#0F1729]' : iconCls}`} />
                  </RotateNavButton>

                  {/* Center dot button */}
                  <button className={`border-[1.5px] flex items-center justify-center rounded-lg w-12.5 h-12.5 ${connected ? 'border-[#0284C5]/30 bg-[#0F1C2D]' : 'border-[#C8D3DF]/10 bg-[#0F1C2D]'}`}>
                    <span className={`w-2 h-2 rounded-full ${connected ? 'bg-[#0284C5]' : 'bg-[#627084]'}`}></span>
                  </button>

                  <RotateNavButton
                    onClick={() => handleRotate(0)}
                    className={`transition-all duration-75 ${isActive('ArrowRight') ? 'scale-95 bg-[#3CE89B] shadow-[0_0_9.6px_#3CE89B40,0_0_5px_#3CE89B40] bg-linear-to-r from-[#3CE89B] to-[#228257]' : ''} ${connected ? 'border-[#C8D3DF] bg-[#324052]/50 cursor-pointer hover:bg-[#3CE89B] hover:border-0 hover:shadow-[0_0_9.6px_#3CE89B40,0_0_5px_#3CE89B40] hover:bg-linear-to-r hover:from-[#3CE89B] hover:to-[#228257] transition-all' : 'border-[#2A3646] bg-[#0F1C2D]'}`}>
                    <RotateCw className={`w-4.5 h-4.5 stroke-2 transition-colors ${isActive('ArrowRight') ? 'stroke-[#0F1729]' : iconCls}`} />
                  </RotateNavButton>

                  {/* Row 3 — Down */}
                  <div />
                  <ArrowNavButton className={`transition-all duration-75 ${isActive('ArrowDown') ? 'bg-[#3CE89B] shadow-[0_0_9.6px_#3CE89B40,0_0_5px_#3CE89B40] bg-linear-to-r from-[#3CE89B] to-[#228257] transition-all scale-95' : ''} ${connected ? 'border-[#C8D3DF] bg-[#324052]/50 cursor-pointer hover:bg-[#3CE89B] hover:border-0 hover:shadow-[0_0_9.6px_#3CE89B40,0_0_5px_#3CE89B40] hover:bg-linear-to-r hover:from-[#3CE89B] hover:to-[#228257] transition-all' : 'border-[#2A3646] bg-[#0F1C2D]'}  `}>
                    <ArrowDown className={`w-4.5 h-4.5 stroke-2 transition-colors ${isActive('ArrowDown') ? 'stroke-[#0F1729]' : iconCls}`} />
                  </ArrowNavButton>
                  <div />
                </div>
              </div>
            </div>

            {/* Gait & Speed panel */}
            <div className={`flex flex-1 flex-col rounded-lg p-4 gap-3 ${panelCls}`}>
              <div className={`text-[14px] tracking-[4px] pb-2 ${titleCls}`}>Gait &amp; Speed</div>
 
              {/* Gait buttons */}
              <div className='grid grid-cols-2 grid-rows-2 gap-0.5'>
              {['WALK', 'WIGGLE', 'CRAWL', 'STAND'].map((label) => (
                <MovButton
                  key={label}
                  onClick={() => handleGait(label)}
                  className={`${
                    activeGait === label
                      ? 'bg-linear-to-r from-[#3CE89B] to-[#228257] text-[#0F1729] border-0 shadow-[0_0_9.6px_#3CE89B40]'
                      : connected
                        ? 'border-[#C8D3DF] bg-[#324052]/50 cursor-pointer text-[#0284C5] hover:bg-[#3CE89B] hover:border-0 hover:text-[#0F1729] hover:shadow-[0_0_9.6px_#3CE89B40,0_0_5px_#3CE89B40] hover:bg-linear-to-r hover:from-[#3CE89B] hover:to-[#228257] transition-all'
                        : 'border-[#2A3646] bg-[#0F1C2D] text-[#627084]'
                  }`}
                >
                  {label}
                </MovButton>
              ))}
              </div>

              {/* Speed slider */}
              <div>
                <div className={`flex justify-between items-center text-[14px] tracking-[2px] ${titleCls}`}>
                  <span>Speed</span>
                </div>
                <SpeedSlider
                  value={speed}
                  onChange={handleSpeed}
                  className={`${connected ? 'bg-[#43567F]' : 'bg-[#43567F66]'}`}
                />
              </div>
            </div>
          </div>

          {/* RIGHT COLUMN: Camera Feed */}
          <div className={`flex flex-col rounded-[10px] p-4 h-full ${panelCls}`}>

            {/* Camera Feed title + REC indicator */}
            <div className='flex items-center justify-between text-[14px] tracking-[4px] pb-2'>
              <span className={titleCls}>Camera feed</span>
              <div className='flex items-center gap-1.5'>
                <span className={`w-2 h-2 rounded-full ${camFrame ? 'bg-[#DB2424] animate-pulse' : 'bg-[#627084]'}`}></span>
                <span className={camFrame ? 'text-[#DB2424]' : 'text-[#627084]'}>REC</span>
              </div>
            </div>

            <div className={`h-91 rounded-lg overflow-hidden ${
              connected
                ? 'border border-[#33CCCC]/60 '   // connected
                : 'border border-[#17CFBF]/40'    // disconnected
            }`}>

              
              {camFrame ? (
                <img src={camFrame} alt='camera feed' className='w-full h-full object-cover' />
              ) : (
                <div className='w-full h-full flex flex-col items-center justify-center bg-[#0F1C2D] gap-2'>
                  <div className={`w-7 h-7 flex items-center justify-center ${
                    connected
                      ? 'bg-[#0284C5] shadow-[0_0_10px_#17CFBF80,0_0_20px_#17CFBF40]'
                      : 'shadow-[0_0_10px_#17CFBF30]'
                  }`}>
                    <Camera className={`w-6 h-6 stroke-1.5 ${connected ? 'stroke-white' : 'stroke-[#FFFFFF]/50'}`} />
                  </div>
                  <p className={`font-space text-[11px] uppercase ${
                    connected
                      ? 'text-[#0284C5] [text-shadow:0_0_10px_#17CFBF80,0_0_20px_#17CFBF40]'
                      : 'text-[#627084] [text-shadow:0_0_10px_#17CFBF80,0_0_20px_#17CFBF40]'
                  }`}>
                    No Signal
                  </p>
                </div>
              )}
            </div>
          </div>
        </div>
      </div>
    </>
  );
}

export default App;