import os
import sys
import pandas as pd
import xml.etree.ElementTree as et

def read_dataframe_from_csv(source_dir: str, file: str) -> pd.DataFrame:
    res = pd.read_csv(
        f"{source_dir}/{file}",     
        parse_dates= {"datetime": [5, 4]},            
        infer_datetime_format= True,
        dayfirst= True)                

    res.columns = ["datetime", "latitude", "longitude", "sog", "cog"]

    return res.set_index("datetime")

def dataframe_to_gpx_file(df: pd.DataFrame, filePath: str) -> None:    
    gpx = et.Element("gpx", attrib= {"creator": "MS", "xmlns": "http://www.topografix.com/GPX/1/1"})  
    trk = et.SubElement(gpx, "trk")
    name = et.SubElement(trk, "name")
    name.text = "merged tracks"
    trkseg = et.SubElement(trk, "trkseg")

    def create_trkpt(lat: float, lon: float, time_index: pd.Timestamp) -> None:
        trkpt = et.SubElement(trkseg, "trkpt", attrib= {"lat": str(lat), "lon": str(lon)})
        time = et.SubElement(trkpt, "time")
        time.text = str(time_index)

    for index, row in df.iterrows():                
        create_trkpt(row["latitude"], row["longitude"], index)           
    
    tree = et.ElementTree(gpx)
    et.indent(tree, space= "\t", level= 0)

    with open(filePath, "wb") as file:
        tree.write(file, encoding="utf-8")    

def main() -> None:
    args = sys.argv
    
    assert len(args) == 3, "expecting 3 parameters"    

    source_dir, res_file = args[-2:]        
    df = pd.concat(map(lambda file: read_dataframe_from_csv(source_dir, file), os.listdir(source_dir))).sort_index()
        
    dataframe_to_gpx_file(df, res_file)    

if __name__ == "__main__":
    main()