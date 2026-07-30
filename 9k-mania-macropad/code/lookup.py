# Magnetic field behaves inversely to the square of distance
# Total travel for Magnetic Jade Attraction is 3.2mm
total_travel = 3.2
points = 17

print("RAW_ADC_PCT , TRUE_TRAVEL_MM")
for i in range(points):
    # Normalized position index from 0.0 (rest) to 1.0 (bottom out)
    raw_pct = i / (points - 1)
    
    # Apply a standard inverse-curve modifier to simulate flux density drop-off
    # (Adjust the exponent factor based on real sensor behavior)
    linearized_pct = 1.0 - ((1.0 - raw_pct) ** 2.5) 
    
    true_mm = linearized_pct * total_travel
    print(f"{raw_pct * 100:6.0f}%    , {true_mm:.2f} mm")
