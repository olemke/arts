# sensor_descriptions/sensor_amsub.arts
dlos = [
    -48.95,
    -47.85,
    -46.75,
    -45.65,
    -44.55,
    -43.45,
    -42.35,
    -41.25,
    -40.15,
    -39.05,
    -37.95,
    -36.85,
    -35.75,
    -34.65,
    -33.55,
    -32.45,
    -31.35,
    -30.25,
    -29.15,
    -28.05,
    -26.95,
    -25.85,
    -24.75,
    -23.65,
    -22.55,
    -21.45,
    -20.35,
    -19.25,
    -18.15,
    -17.05,
    -15.95,
    -14.85,
    -13.75,
    -12.65,
    -11.55,
    -10.45,
    -9.35,
    -8.25,
    -7.15,
    -6.05,
    -4.95,
    -3.85,
    -2.75 - 1.65,
    -0.55,
]

ws.MatrixSet(ws.antenna_dlos, np.array([dlos]).T)

mm = [
    [89.00e9, 0.90e9, 0.0, 1000e6],
    [150.00e9, 0.90e9, 0.0, 1000e6],
    [183.31e9, 1.00e9, 0.0, 500e6],
    [183.31e9, 3.00e9, 0.0, 1000e6],
    [183.31e9, 7.00e9, 0.0, 2000e6],
]

ws.MatrixSet(ws.met_mm_backend, np.array(mm))

ws.ArrayOfStringSet(
    ws.met_mm_polarisation,
    [
        "AMSU-V",  # 0 (H1)
        "AMSU-V",  # 1 (H2)
        "AMSU-V",  # 2 (H3)
        "AMSU-V",  # 3 (H4)
        "AMSU-V",  # 4 (H5)
    ],
)

# ws.VectorSet( ws.met_mm_antenna, "" )
ws.Touch(ws.met_mm_antenna)

# Number of frequencies for first accuracy (fast)
ws.ArrayOfIndexSet(
    ws.freq_number_tmp,
    [1, 1, 1, 1, 1],  # 0 (16)  # 1 (17)  # 2 (18)  # 3 (19)  # 4 (20)
)

ws.Append(ws.met_mm_available_accuracies, ws.freq_number_tmp)

# Number of frequencies for second accuracy (normal)
ws.ArrayOfIndexSet(
    ws.freq_number_tmp,
    [1, 2, 2, 2, 3],  # 0 (16)  # 1 (17)  # 2 (18)  # 3 (19)  # 4 (20)
)

ws.Append(ws.met_mm_available_accuracies, ws.freq_number_tmp)

# Number of frequencies for third accuracy (high)
ws.ArrayOfIndexSet(
    ws.freq_number_tmp,
    [1, 18, 20, 7, 10],  # 0 (16)  # 1 (17)  # 2 (18)  # 3 (19)  # 4 (20)
)

ws.Append(ws.met_mm_available_accuracies, ws.freq_number_tmp)

# Number of frequencies for fourth accuracy (reference)
ws.ArrayOfIndexSet(
    ws.freq_number_tmp,
    [2, 23, 67, 19, 25],  # 0 (16)  # 1 (17)  # 2 (18)  # 3 (19)  # 4 (20)
)

ws.Append(ws.met_mm_available_accuracies, ws.freq_number_tmp)


ws.VectorSet(ws.freq_spacing_tmp, np.array([10e9, 1e9, 1e9, 1e9]))

ws.Extract(ws.met_mm_freq_number, ws.met_mm_available_accuracies, ws.met_mm_accuracy)
ws.Extract(ws.current_spacing, ws.freq_spacing_tmp, ws.met_mm_accuracy)

ws.nrowsGet(ws.met_mm_nchannels, ws.met_mm_backend)
ws.VectorSetConstant(ws.met_mm_freq_spacing, ws.nrows, ws.current_spacing)
ws.Delete(ws.current_spacing)
