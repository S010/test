const TIME_SCALE = 1.0;

var clock = new THREE.Clock()
var scene = new THREE.Scene();
var camera = new THREE.PerspectiveCamera(75, window.innerWidth / window.innerHeight, 0.1, 1000);
var renderer = new THREE.WebGLRenderer();

renderer.setSize(window.innerWidth, window.innerHeight);
renderer.physicallyCorrectLights = true;
document.body.appendChild(renderer.domElement);

camera.position.z = 5;
camera.lookAt(0, 0, 0);

var light = new THREE.HemisphereLight(0xffffbb, 0x080820, 10);
scene.add(light);

var geometry = new THREE.BoxGeometry(1, 1, 1);
var material = new THREE.MeshStandardMaterial({color: 0x0033ff});
var cube = new THREE.Mesh(geometry, material);
scene.add(cube);

var t = 0.0;

function sin(t) {
	return Math.sin(2*Math.PI*t*TIME_SCALE);
}

function cos(t) {
	return Math.cos(2*Math.PI*t*TIME_SCALE);
}

function main_loop() {
	td = clock.getDelta()
	t += td;

	var cosine = Math.cos(2*Math.PI*t*TIME_SCALE);

	cube.position.x = sin(t*0.3) * 4;
	cube.position.y = sin(t*0.2) * sin(t*0.2) * 2;
	cube.position.z = sin(t*0.22) * cos(t*0.23) * 2;

	cube.rotateX(sin(t*0.02)*td);
	cube.rotateY(cos(t*0.2)*td);
	cube.rotateZ(sin(t*0.1)*td);

	requestAnimationFrame(main_loop);
	renderer.render(scene, camera);
}

function start_main_loop() {
	clock.getDelta();
	main_loop();
}

start_main_loop();

