// Database Paths
var dataFloatPath = 'signos_vitales/frecuencia_cardiaca';
var dataIntPath = 'signos_vitales/oxigeno';

// Get references to database paths
var databaseFloat = firebase.database().ref(dataFloatPath);
var databaseInt = firebase.database().ref(dataIntPath);

// Variables to save database values
var floatReading = 0;
var intReading = 0;

// Attach a callback to retrieve updates for float data
databaseFloat.on('value', function(snapshot) {
    floatReading = snapshot.val();
    console.log("Float Reading:", floatReading);
    // Asegúrate de que este ID coincida con tu HTML
    document.getElementById("ritmo-cardiaco").innerText = floatReading;
}, function(error) {
    console.error("Error reading float data:", error);
});

// Attach a callback to retrieve updates for int data
databaseInt.on('value', function(snapshot) {
    intReading = snapshot.val();
    console.log("Int Reading:", intReading);
    // Asegúrate de que este ID coincida con tu HTML
    document.getElementById("saturacion-oxigeno").innerText = intReading;
}, function(error) {
    console.error("Error reading int data:", error);
});

