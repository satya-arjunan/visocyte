#!/usr/bin/env python

import pandas as pd
df = pd.read_csv('_Position.csv', skiprows=3)
df = df.sort_values(by=['Time'])
df.to_csv('time_sorted_Position.csv', index=False)
