import json
import pandas as pd
import os

# 1. Load the custom geo json data
with open('custom.geo.json', 'r', encoding='utf-8') as f:
    geo_data = json.load(f)

# 2. Build the mapping dictionary
json_alias_to_canonical = {}
priority_fields = [
    'name', 'name_long', 'formal_en', 'name_alt', 
    'name_ciawf', 'name_sort', 'brk_name', 'abbrev'
]

for feature in geo_data['features']:
    props = feature['properties']
    canonical_name = props.get('name')
    
    for field in priority_fields:
        alias = props.get(field)
        if alias and isinstance(alias, str):
            if alias not in json_alias_to_canonical:
                json_alias_to_canonical[alias] = canonical_name

# 3. Add Manual Fixes
manual_fixes = {
    "Bolivia (Plurinational State of)": "Bolivia",
    "Dem. People's Republic of Korea": "North Korea",
    "Korea, Republic of": "South Korea",
    "Iran (Islamic Republic of)": "Iran",
    "Macedonia": "North Macedonia",
    "Venezuela, Bolivarian Republic of": "Venezuela",
    "Viet Nam": "Vietnam",
    "Republic of Moldova": "Moldova",
    "Syrian Arab Republic": "Syria",
    "Lao People's Democratic Republic": "Laos",
    "United Republic of Tanzania": "Tanzania",
    "Democratic Republic of the Congo": "Dem. Rep. Congo",
    "Congo": "Congo",
    "Swaziland": "eSwatini",
    "The Gambia": "Gambia",
    "Cape Verde": "Cabo Verde"
}
json_alias_to_canonical.update(manual_fixes)

# 4. Process and Overwrite ALL CSV files
all_files = os.listdir('.')
csv_files = [f for f in all_files if f.endswith('.csv')]

print(f"Processing files: {csv_files}")

for csv_file in csv_files:
    try:
        df = pd.read_csv(csv_file)
        
        # Check for 'Country' column (case-insensitive)
        country_col = None
        for col in df.columns:
            if col.lower() == 'country':
                country_col = col
                break
        
        if country_col:
            # Apply mapping
            df[country_col] = df[country_col].apply(lambda x: json_alias_to_canonical.get(x, x))
            
            # Save back to the SAME file (Overwrite)
            df.to_csv(csv_file, index=False)
            print(f"  -> Overwrote {csv_file}")
        else:
            print(f"  -> Skipped {csv_file} (No 'Country' column)")
            
    except Exception as e:
        print(f"  -> Error processing {csv_file}: {e}")