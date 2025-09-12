# Reproducibility

1. Create a virtual environment to not allow slither mess with you or vice-versa: python3 -m venv <path>/venv
2. Activate the environment (Obviously, otherwise, why would you have created it): source ./venv/bin/activate
3. Upgrade the pip (if you are also working with old pip like me): python3 -m pip install --upgrade pip
4. Here comes the devil that caused the trouble for step 1-3: python3 -m pip install slither-analyzer
5. Lovely command (Don't say that you want me to write cd <location to tracegen.py>: python3 tracegen.py 
6. Voila. Did you see the magic with name analysis.trace?
