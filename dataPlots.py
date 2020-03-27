import boto3
import pandas as pd
from matplotlib import pyplot as plt

# create IoT Analytics client
client = boto3.client('iotanalytics')

#after running the code portion above run the code below with your own dataset


dataset = "YOUR_DATASET_HERE"
dataset_url = client.get_dataset_content(datasetName = dataset)['entries'][0]['dataURI']

# start working with the data

df = pd.read_csv(dataset_url)
df.timestamp = pd.to_datetime(pd.to_numeric(df.timestamp), unit='ms')
df.set_index(df.timestamp, inplace=True)
df.sort_values('timestamp', inplace=True)

fig, ax = plt.subplots()
df.temperature.plot(legend=True)
df.humidity.plot(legend=True)
plt.show()
