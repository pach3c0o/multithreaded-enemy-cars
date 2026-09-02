/**
 * Network
 * Opens a WebSocket to the C++ backend and keeps the latest enemy-car list.
 *
 * The backend sends one JSON message per tick:
 *   { "lanes": 3, "height": 840, "cars": [ { "id", "lane", "y", "variant" }, ... ] }
 */
class Network {
  constructor(url) {
    this.url = url;
    this.connected = false;
    this.cars = [];
  }

  connect() {
    const socket = new WebSocket(this.url);

    socket.onopen = () => {
      this.connected = true;
      console.log('[network] connected to', this.url);
    };

    socket.onmessage = (event) => {
      const data = JSON.parse(event.data);
      this.cars = data.cars;
    };

    socket.onclose = () => {
      this.connected = false;
      this.cars = [];
      console.log('[network] disconnected, retrying in 1s');
      setTimeout(() => this.connect(), 1000);
    };

    socket.onerror = () => {
      socket.close();
    };
  }

  getCars() {
    return this.cars;
  }
}
