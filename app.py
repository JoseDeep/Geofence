from flask import Flask, request, jsonify, render_template
from shapely.geometry import Point, Polygon

app = Flask(__name__)

# Variable to hold the geofence polygon and its coordinates
geofence_polygon = None
geofence_coordinates = []

@app.route('/')
def home():
    return render_template('map.html')

@app.route('/set_geofence', methods=['POST'])
def set_geofence():
    global geofence_polygon, geofence_coordinates
    data = request.json
    coordinates = data.get('coordinates')  # List of [lat, lng] pairs

    if not coordinates:
        return jsonify({'error': 'Invalid coordinates.'}), 400

    # Save coordinates and create a polygon
    geofence_coordinates = coordinates
    geofence_polygon = Polygon([(lng, lat) for lat, lng in coordinates])
    return jsonify({'message': 'Geofence set successfully!'})

@app.route('/get_geofence', methods=['GET'])
def get_geofence():
    if not geofence_coordinates:
        return jsonify({'error': 'Geofence not set.'}), 404

    return jsonify({'coordinates': geofence_coordinates})

@app.route('/check_location', methods=['POST'])
def check_location():
    global geofence_polygon
    if geofence_polygon is None:
        return jsonify({'error': 'Geofence not set.'}), 400

    data = request.json
    lat = data.get('latitude')
    lng = data.get('longitude')

    if lat is None or lng is None:
        return jsonify({'error': 'Invalid input. Latitude and longitude are required.'}), 400

    point = Point(lng, lat)
    inside = geofence_polygon.contains(point)
    return jsonify({'inside_geofence': inside})

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)
