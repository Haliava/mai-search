from flask import Flask, render_template, request
import subprocess
import json
import os

app = Flask(__name__)

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/search')
def search():
    query = request.args.get('q', '')
    if not query:
        return render_template('index.html')

    try:
        result = subprocess.run(
            ['./search_engine', query], 
            capture_output=True, 
            text=True, 
            encoding='utf-8'
        )
        
        if result.returncode != 0:
             return f"Engine Error: {result.stderr}"

        data = json.loads(result.stdout)
        
        return render_template('index.html', 
                               query=query, 
                               results=data['results'], 
                               count=data['count'], 
                               time=data['time_sec'])

    except Exception as e:
        return f"Error: {str(e)}"

if __name__ == '__main__':
    app.run(debug=True, port=5000)