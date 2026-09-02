// Game configuration

const GameConfig = {
  // Display size (must match WINDOW_HEIGHT / lane count in the backend)
  GAME_WIDTH: 640,
  GAME_HEIGHT: 840,
  LANES: 3,

  // Gameplay
  GAME_SPEED: 4,        // road scroll speed
  PLAYER_SPEED: 3,      // player car movement speed

  // Colors / rendering
  BACKGROUND_COLOR: 0x404040,
  ROAD_COLOR: 0x8a8a8a,
  RESOLUTION: 2,

  // Backend WebSocket: same host as the page, port 5000
  BACKEND_URL: 'ws://' + window.location.hostname + ':5000',
};

// Key constants
const Keys = {
  ARROW_LEFT: 'ArrowLeft',
  ARROW_UP: 'ArrowUp',
  ARROW_RIGHT: 'ArrowRight',
  ARROW_DOWN: 'ArrowDown',
  SPACE: 'Space',
  KEY_M: 'KeyM',
};
