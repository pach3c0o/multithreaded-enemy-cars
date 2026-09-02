/**
 * Main game logic (render-only client)
 *
 * The enemy cars are created and moved by the C++ backend. This client just:
 *   - connects to the backend over a WebSocket
 *   - draws one sprite per enemy car at the position the backend reports
 *   - lets the player move their own car and checks collisions
 */

// Axis-aligned overlap test with a small margin so it feels fair
function carsOverlap(a, b) {
  const m = 6;
  return (
    a.x + m < b.x + b.width - m &&
    a.x + a.width - m > b.x + m &&
    a.y + m < b.y + b.height - m &&
    a.y + a.height - m > b.y + m
  );
}

const keyboard = new KeyBoard().addEvents();
const audioManager = new AudioManager();

window.onload = function () {
  const width = GameConfig.GAME_WIDTH;
  const height = GameConfig.GAME_HEIGHT;

  const app = new PIXI.Application({
    width: width,
    height: height,
    backgroundColor: GameConfig.BACKGROUND_COLOR,
    resolution: GameConfig.RESOLUTION,
  });

  // Connect to the backend
  const network = new Network(GameConfig.BACKEND_URL);
  network.connect();

  // Load assets
  app.loader.add('player', 'assets/BlackOut.png');
  app.loader.add('enemy1', 'assets/RedStrip.png');
  app.loader.add('enemy2', 'assets/BlueStrip.png');
  app.loader.add('enemy3', 'assets/GreenStrip.png');
  app.loader.add('enemy4', 'assets/PinkStrip.png');
  app.loader.add('enemy5', 'assets/WhiteStrip.png');
  for (let i = 0; i <= 63; i++) {
    app.loader.add(`exp-${i}`, `assets/explosion/frame00${(i < 10 ? '0' : '')}${i}.png`);
  }
  app.loader.onComplete.add(startGame);
  app.loader.load();

  function startGame() {
    document.getElementById('game').appendChild(app.view);
    app.stage.sortableChildren = true;

    // Road / background
    const scenario = new GameBackground(width, height, GameConfig.GAME_SPEED, GameConfig.LANES);
    app.stage.addChild(scenario.container);

    // Map a backend lane index (0..LANES-1) to a screen x (center of the lane)
    const laneWidth = (scenario.xRoadEnd - scenario.xRoadStart) / GameConfig.LANES;
    function laneToX(lane) {
      return scenario.xRoadStart + laneWidth * lane + laneWidth / 2;
    }

    // Player car
    const playerCar = new PlayerCar(
      app,
      scenario.xRoadStart,
      scenario.xRoadEnd,
      height,
      GameConfig.PLAYER_SPEED
    ).setPosition(width / 2, height / 2);
    app.stage.addChild(playerCar.sprite);
    app.stage.addChild(playerCar.explosion);

    // One sprite per backend car id
    const enemySprites = {};

    // Small status line (top-left)
    const statusText = new PIXI.Text('', {
      fontFamily: 'Arial',
      fontSize: 14,
      fill: 0xffffff,
      stroke: 'black',
      strokeThickness: 3,
    });
    statusText.x = 12;
    statusText.y = 12;
    statusText.zIndex = 100;
    app.stage.addChild(statusText);

    // Start / game over overlays
    const startText = new PIXI.Text('PRESS AN ARROW KEY TO START', {
      fontFamily: 'Arial',
      fontSize: 26,
      fill: 0xffea00,
      align: 'center',
      stroke: 'black',
      strokeThickness: 5,
    });
    startText.anchor.set(0.5);
    startText.position.set(width / 2, height / 2);
    startText.zIndex = 200;
    app.stage.addChild(startText);

    const gameOverText = new PIXI.Text('GAME OVER\nPRESS SPACE TO RESTART', {
      fontFamily: 'Arial',
      fontSize: 30,
      fill: 0xff0000,
      align: 'center',
      stroke: 'black',
      strokeThickness: 6,
    });
    gameOverText.anchor.set(0.5);
    gameOverText.position.set(width / 2, height / 2);
    gameOverText.zIndex = 200;
    gameOverText.visible = false;
    app.stage.addChild(gameOverText);

    let started = false;
    let lost = false;

    function restart() {
      started = true;
      lost = false;
      gameOverText.visible = false;
      playerCar.setPosition(width / 2, height / 2);
      playerCar.explosion.visible = false;
      audioManager.startBackgroundMusic();
    }

    window.addEventListener('keydown', (event) => {
      const arrows = [Keys.ARROW_UP, Keys.ARROW_DOWN, Keys.ARROW_LEFT, Keys.ARROW_RIGHT];

      if (event.code === Keys.KEY_M) {
        audioManager.toggleMute();
      } else if (!started && !lost && arrows.includes(event.code)) {
        started = true;
        startText.visible = false;
        audioManager.startBackgroundMusic();
      } else if (lost && event.code === Keys.SPACE) {
        restart();
      }
    });

    // Game loop
    app.ticker.add(() => {
      scenario.animate();
      statusText.text = network.connected ? 'BACKEND: connected' : 'BACKEND: connecting...';

      // Sync enemy sprites with the latest backend snapshot
      const cars = network.getCars();
      const seen = {};
      for (let i = 0; i < cars.length; i++) {
        const car = cars[i];
        seen[car.id] = true;

        let enemy = enemySprites[car.id];
        if (!enemy) {
          enemy = new EnemyCar(app, car.variant, laneToX);
          enemySprites[car.id] = enemy;
          app.stage.addChild(enemy.sprite);
        }
        enemy.updateFromServer(car.lane, car.y);
      }

      // Drop sprites for cars the backend no longer sends
      for (const id in enemySprites) {
        if (!seen[id]) {
          app.stage.removeChild(enemySprites[id].sprite);
          delete enemySprites[id];
        }
      }

      if (!started || lost) {
        return;
      }

      // Player movement
      if (keyboard.isKeyPress(Keys.ARROW_UP)) playerCar.moveUp();
      if (keyboard.isKeyPress(Keys.ARROW_DOWN)) playerCar.moveDown();
      if (keyboard.isKeyPress(Keys.ARROW_LEFT)) playerCar.moveLeft();
      if (keyboard.isKeyPress(Keys.ARROW_RIGHT)) playerCar.moveRight();

      // Collision with any enemy car
      const playerBounds = playerCar.sprite.getBounds();
      for (const id in enemySprites) {
        const enemyBounds = enemySprites[id].sprite.getBounds();
        if (carsOverlap(playerBounds, enemyBounds)) {
          playerCar.explode();
          lost = true;
          gameOverText.visible = true;
          audioManager.playCollision();
          audioManager.stopBackgroundMusic();
        }
      }
    });
  }
};
