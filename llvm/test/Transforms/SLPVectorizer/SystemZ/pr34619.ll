; RUN: opt < %s -mtriple=systemz-unknown -mcpu=z13 -slp-vectorizer
%struct.ImageParameters.11.131.155.323.491.899.923.947.971.995.1043.1067.1091.1115.1139.1187.1235.1307.1331.1355.1379.1595.1691.1883.1907.2027.2099.2387.2411.2507.2531.2771.3179.3203.3227.3251.3275.3443.3467.3683.4115.4379.4859.6058.10.21.32.43.54.549.560 = type { i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, float, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i8**, i8**, i32, i32***, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, [9 x [16 x [16 x i16]]], [5 x [16 x [16 x i16]]], [9 x [8 x [8 x i16]]], [2 x [4 x [16 x [16 x i16]]]], [16 x [16 x i16]], [16 x [16 x i32]], i32****, i32***, i32***, i32***, i32****, i32****, %struct.Picture.8.128.152.320.488.896.920.944.968.992.1040.1064.1088.1112.1136.1184.1232.1304.1328.1352.1376.1592.1688.1880.1904.2024.2096.2384.2408.2504.2528.2768.3176.3200.3224.3248.3272.3440.3464.3680.4112.4376.4856.6055.7.18.29.40.51.546.557*, %struct.Slice.7.127.151.319.487.895.919.943.967.991.1039.1063.1087.1111.1135.1183.1231.1303.1327.1351.1375.1591.1687.1879.1903.2023.2095.2383.2407.2503.2527.2767.3175.3199.3223.3247.3271.3439.3463.3679.4111.4375.4855.6054.6.17.28.39.50.545.556*, %struct.macroblock.9.129.153.321.489.897.921.945.969.993.1041.1065.1089.1113.1137.1185.1233.1305.1329.1353.1377.1593.1689.1881.1905.2025.2097.2385.2409.2505.2529.2769.3177.3201.3225.3249.3273.3441.3465.3681.4113.4377.4857.6056.8.19.30.41.52.547.558*, i32*, i32*, i32, i32, i32, i32, [4 x [4 x i32]], i32, i32, i32, i32, i32, double, i32, i32, i32, i32, i16******, i16******, i16******, i16******, [15 x i16], i32, i32, i32, i32, i32, i32, i32, i32, [6 x [32 x i32]], i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, [1 x i32], i32, i32, [2 x i32], i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, %struct.DecRefPicMarking_s.10.130.154.322.490.898.922.946.970.994.1042.1066.1090.1114.1138.1186.1234.1306.1330.1354.1378.1594.1690.1882.1906.2026.2098.2386.2410.2506.2530.2770.3178.3202.3226.3250.3274.3442.3466.3682.4114.4378.4858.6057.9.20.31.42.53.548.559*, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, double**, double***, i32***, double**, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, [3 x [2 x i32]], [2 x i32], i32, i32, i16, i32, i32, i32, i32, i32 }
%struct.Picture.8.128.152.320.488.896.920.944.968.992.1040.1064.1088.1112.1136.1184.1232.1304.1328.1352.1376.1592.1688.1880.1904.2024.2096.2384.2408.2504.2528.2768.3176.3200.3224.3248.3272.3440.3464.3680.4112.4376.4856.6055.7.18.29.40.51.546.557 = type { i32, i32, [100 x %struct.Slice.7.127.151.319.487.895.919.943.967.991.1039.1063.1087.1111.1135.1183.1231.1303.1327.1351.1375.1591.1687.1879.1903.2023.2095.2383.2407.2503.2527.2767.3175.3199.3223.3247.3271.3439.3463.3679.4111.4375.4855.6054.6.17.28.39.50.545.556*], i32, float, float, float }
%struct.Slice.7.127.151.319.487.895.919.943.967.991.1039.1063.1087.1111.1135.1183.1231.1303.1327.1351.1375.1591.1687.1879.1903.2023.2095.2383.2407.2503.2527.2767.3175.3199.3223.3247.3271.3439.3463.3679.4111.4375.4855.6054.6.17.28.39.50.545.556 = type { i32, i32, i32, i32, i32, i32, %struct.datapartition.3.123.147.315.483.891.915.939.963.987.1035.1059.1083.1107.1131.1179.1227.1299.1323.1347.1371.1587.1683.1875.1899.2019.2091.2379.2403.2499.2523.2763.3171.3195.3219.3243.3267.3435.3459.3675.4107.4371.4851.6050.2.13.24.35.46.541.552*, %struct.MotionInfoContexts.5.125.149.317.485.893.917.941.965.989.1037.1061.1085.1109.1133.1181.1229.1301.1325.1349.1373.1589.1685.1877.1901.2021.2093.2381.2405.2501.2525.2765.3173.3197.3221.3245.3269.3437.3461.3677.4109.4373.4853.6052.4.15.26.37.48.543.554*, %struct.TextureInfoContexts.6.126.150.318.486.894.918.942.966.990.1038.1062.1086.1110.1134.1182.1230.1302.1326.1350.1374.1590.1686.1878.1902.2022.2094.2382.2406.2502.2526.2766.3174.3198.3222.3246.3270.3438.3462.3678.4110.4374.4854.6053.5.16.27.38.49.544.555*, i32, i32*, i32*, i32*, i32, i32*, i32*, i32*, i32 (i32)*, [3 x [2 x i32]] }
%struct.datapartition.3.123.147.315.483.891.915.939.963.987.1035.1059.1083.1107.1131.1179.1227.1299.1323.1347.1371.1587.1683.1875.1899.2019.2091.2379.2403.2499.2523.2763.3171.3195.3219.3243.3267.3435.3459.3675.4107.4371.4851.6050.2.13.24.35.46.541.552 = type { %struct.Bitstream.1.121.145.313.481.889.913.937.961.985.1033.1057.1081.1105.1129.1177.1225.1297.1321.1345.1369.1585.1681.1873.1897.2017.2089.2377.2401.2497.2521.2761.3169.3193.3217.3241.3265.3433.3457.3673.4105.4369.4849.6048.0.11.22.33.44.539.550*, %struct.EncodingEnvironment.2.122.146.314.482.890.914.938.962.986.1034.1058.1082.1106.1130.1178.1226.1298.1322.1346.1370.1586.1682.1874.1898.2018.2090.2378.2402.2498.2522.2762.3170.3194.3218.3242.3266.3434.3458.3674.4106.4370.4850.6049.1.12.23.34.45.540.551, %struct.EncodingEnvironment.2.122.146.314.482.890.914.938.962.986.1034.1058.1082.1106.1130.1178.1226.1298.1322.1346.1370.1586.1682.1874.1898.2018.2090.2378.2402.2498.2522.2762.3170.3194.3218.3242.3266.3434.3458.3674.4106.4370.4850.6049.1.12.23.34.45.540.551 }
%struct.Bitstream.1.121.145.313.481.889.913.937.961.985.1033.1057.1081.1105.1129.1177.1225.1297.1321.1345.1369.1585.1681.1873.1897.2017.2089.2377.2401.2497.2521.2761.3169.3193.3217.3241.3265.3433.3457.3673.4105.4369.4849.6048.0.11.22.33.44.539.550 = type { i32, i32, i8, i32, i32, i8, i8, i32, i32, i8*, i32 }
%struct.EncodingEnvironment.2.122.146.314.482.890.914.938.962.986.1034.1058.1082.1106.1130.1178.1226.1298.1322.1346.1370.1586.1682.1874.1898.2018.2090.2378.2402.2498.2522.2762.3170.3194.3218.3242.3266.3434.3458.3674.4106.4370.4850.6049.1.12.23.34.45.540.551 = type { i32, i32, i32, i32, i32, i8*, i32*, i32, i32 }
%struct.MotionInfoContexts.5.125.149.317.485.893.917.941.965.989.1037.1061.1085.1109.1133.1181.1229.1301.1325.1349.1373.1589.1685.1877.1901.2021.2093.2381.2405.2501.2525.2765.3173.3197.3221.3245.3269.3437.3461.3677.4109.4373.4853.6052.4.15.26.37.48.543.554 = type { [3 x [11 x %struct.BiContextType.4.124.148.316.484.892.916.940.964.988.1036.1060.1084.1108.1132.1180.1228.1300.1324.1348.1372.1588.1684.1876.1900.2020.2092.2380.2404.2500.2524.2764.3172.3196.3220.3244.3268.3436.3460.3676.4108.4372.4852.6051.3.14.25.36.47.542.553]], [2 x [9 x %struct.BiContextType.4.124.148.316.484.892.916.940.964.988.1036.1060.1084.1108.1132.1180.1228.1300.1324.1348.1372.1588.1684.1876.1900.2020.2092.2380.2404.2500.2524.2764.3172.3196.3220.3244.3268.3436.3460.3676.4108.4372.4852.6051.3.14.25.36.47.542.553]], [2 x [10 x %struct.BiContextType.4.124.148.316.484.892.916.940.964.988.1036.1060.1084.1108.1132.1180.1228.1300.1324.1348.1372.1588.1684.1876.1900.2020.2092.2380.2404.2500.2524.2764.3172.3196.3220.3244.3268.3436.3460.3676.4108.4372.4852.6051.3.14.25.36.47.542.553]], [2 x [6 x %struct.BiContextType.4.124.148.316.484.892.916.940.964.988.1036.1060.1084.1108.1132.1180.1228.1300.1324.1348.1372.1588.1684.1876.1900.2020.2092.2380.2404.2500.2524.2764.3172.3196.3220.3244.3268.3436.3460.3676.4108.4372.4852.6051.3.14.25.36.47.542.553]], [4 x %struct.BiContextType.4.124.148.316.484.892.916.940.964.988.1036.1060.1084.1108.1132.1180.1228.1300.1324.1348.1372.1588.1684.1876.1900.2020.2092.2380.2404.2500.2524.2764.3172.3196.3220.3244.3268.3436.3460.3676.4108.4372.4852.6051.3.14.25.36.47.542.553], [4 x %struct.BiContextType.4.124.148.316.484.892.916.940.964.988.1036.1060.1084.1108.1132.1180.1228.1300.1324.1348.1372.1588.1684.1876.1900.2020.2092.2380.2404.2500.2524.2764.3172.3196.3220.3244.3268.3436.3460.3676.4108.4372.4852.6051.3.14.25.36.47.542.553], [3 x %struct.BiContextType.4.124.148.316.484.892.916.940.964.988.1036.1060.1084.1108.1132.1180.1228.1300.1324.1348.1372.1588.1684.1876.1900.2020.2092.2380.2404.2500.2524.2764.3172.3196.3220.3244.3268.3436.3460.3676.4108.4372.4852.6051.3.14.25.36.47.542.553] }
%struct.BiContextType.4.124.148.316.484.892.916.940.964.988.1036.1060.1084.1108.1132.1180.1228.1300.1324.1348.1372.1588.1684.1876.1900.2020.2092.2380.2404.2500.2524.2764.3172.3196.3220.3244.3268.3436.3460.3676.4108.4372.4852.6051.3.14.25.36.47.542.553 = type { i16, i8, i64 }
%struct.TextureInfoContexts.6.126.150.318.486.894.918.942.966.990.1038.1062.1086.1110.1134.1182.1230.1302.1326.1350.1374.1590.1686.1878.1902.2022.2094.2382.2406.2502.2526.2766.3174.3198.3222.3246.3270.3438.3462.3678.4110.4374.4854.6053.5.16.27.38.49.544.555 = type { [2 x %struct.BiContextType.4.124.148.316.484.892.916.940.964.988.1036.1060.1084.1108.1132.1180.1228.1300.1324.1348.1372.1588.1684.1876.1900.2020.2092.2380.2404.2500.2524.2764.3172.3196.3220.3244.3268.3436.3460.3676.4108.4372.4852.6051.3.14.25.36.47.542.553], [4 x %struct.BiContextType.4.124.148.316.484.892.916.940.964.988.1036.1060.1084.1108.1132.1180.1228.1300.1324.1348.1372.1588.1684.1876.1900.2020.2092.2380.2404.2500.2524.2764.3172.3196.3220.3244.3268.3436.3460.3676.4108.4372.4852.6051.3.14.25.36.47.542.553], [3 x [4 x %struct.BiContextType.4.124.148.316.484.892.916.940.964.988.1036.1060.1084.1108.1132.1180.1228.1300.1324.1348.1372.1588.1684.1876.1900.2020.2092.2380.2404.2500.2524.2764.3172.3196.3220.3244.3268.3436.3460.3676.4108.4372.4852.6051.3.14.25.36.47.542.553]], [10 x [4 x %struct.BiContextType.4.124.148.316.484.892.916.940.964.988.1036.1060.1084.1108.1132.1180.1228.1300.1324.1348.1372.1588.1684.1876.1900.2020.2092.2380.2404.2500.2524.2764.3172.3196.3220.3244.3268.3436.3460.3676.4108.4372.4852.6051.3.14.25.36.47.542.553]], [10 x [15 x %struct.BiContextType.4.124.148.316.484.892.916.940.964.988.1036.1060.1084.1108.1132.1180.1228.1300.1324.1348.1372.1588.1684.1876.1900.2020.2092.2380.2404.2500.2524.2764.3172.3196.3220.3244.3268.3436.3460.3676.4108.4372.4852.6051.3.14.25.36.47.542.553]], [10 x [15 x %struct.BiContextType.4.124.148.316.484.892.916.940.964.988.1036.1060.1084.1108.1132.1180.1228.1300.1324.1348.1372.1588.1684.1876.1900.2020.2092.2380.2404.2500.2524.2764.3172.3196.3220.3244.3268.3436.3460.3676.4108.4372.4852.6051.3.14.25.36.47.542.553]], [10 x [5 x %struct.BiContextType.4.124.148.316.484.892.916.940.964.988.1036.1060.1084.1108.1132.1180.1228.1300.1324.1348.1372.1588.1684.1876.1900.2020.2092.2380.2404.2500.2524.2764.3172.3196.3220.3244.3268.3436.3460.3676.4108.4372.4852.6051.3.14.25.36.47.542.553]], [10 x [5 x %struct.BiContextType.4.124.148.316.484.892.916.940.964.988.1036.1060.1084.1108.1132.1180.1228.1300.1324.1348.1372.1588.1684.1876.1900.2020.2092.2380.2404.2500.2524.2764.3172.3196.3220.3244.3268.3436.3460.3676.4108.4372.4852.6051.3.14.25.36.47.542.553]], [10 x [15 x %struct.BiContextType.4.124.148.316.484.892.916.940.964.988.1036.1060.1084.1108.1132.1180.1228.1300.1324.1348.1372.1588.1684.1876.1900.2020.2092.2380.2404.2500.2524.2764.3172.3196.3220.3244.3268.3436.3460.3676.4108.4372.4852.6051.3.14.25.36.47.542.553]], [10 x [15 x %struct.BiContextType.4.124.148.316.484.892.916.940.964.988.1036.1060.1084.1108.1132.1180.1228.1300.1324.1348.1372.1588.1684.1876.1900.2020.2092.2380.2404.2500.2524.2764.3172.3196.3220.3244.3268.3436.3460.3676.4108.4372.4852.6051.3.14.25.36.47.542.553]] }
%struct.macroblock.9.129.153.321.489.897.921.945.969.993.1041.1065.1089.1113.1137.1185.1233.1305.1329.1353.1377.1593.1689.1881.1905.2025.2097.2385.2409.2505.2529.2769.3177.3201.3225.3249.3273.3441.3465.3681.4113.4377.4857.6056.8.19.30.41.52.547.558 = type { i32, i32, i32, [2 x i32], i32, [8 x i32], %struct.macroblock.9.129.153.321.489.897.921.945.969.993.1041.1065.1089.1113.1137.1185.1233.1305.1329.1353.1377.1593.1689.1881.1905.2025.2097.2385.2409.2505.2529.2769.3177.3201.3225.3249.3273.3441.3465.3681.4113.4377.4857.6056.8.19.30.41.52.547.558*, %struct.macroblock.9.129.153.321.489.897.921.945.969.993.1041.1065.1089.1113.1137.1185.1233.1305.1329.1353.1377.1593.1689.1881.1905.2025.2097.2385.2409.2505.2529.2769.3177.3201.3225.3249.3273.3441.3465.3681.4113.4377.4857.6056.8.19.30.41.52.547.558*, i32, [2 x [4 x [4 x [2 x i32]]]], [16 x i8], [16 x i8], i32, i64, [4 x i32], [4 x i32], i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i16, double, i32, i32, i32, i32, i32, i32, i32, i32, i32 }
%struct.DecRefPicMarking_s.10.130.154.322.490.898.922.946.970.994.1042.1066.1090.1114.1138.1186.1234.1306.1330.1354.1378.1594.1690.1882.1906.2026.2098.2386.2410.2506.2530.2770.3178.3202.3226.3250.3274.3442.3466.3682.4114.4378.4858.6057.9.20.31.42.53.548.559 = type { i32, i32, i32, i32, i32, %struct.DecRefPicMarking_s.10.130.154.322.490.898.922.946.970.994.1042.1066.1090.1114.1138.1186.1234.1306.1330.1354.1378.1594.1690.1882.1906.2026.2098.2386.2410.2506.2530.2770.3178.3202.3226.3250.3274.3442.3466.3682.4114.4378.4858.6057.9.20.31.42.53.548.559* }

@dct_luma.m4 = external global [4 x [4 x i32]], align 4

define void @dct_luma() local_unnamed_addr {
entry:
  %add277 = add nsw i32 undef, undef
  store i32 %add277, i32* getelementptr inbounds ([4 x [4 x i32]], [4 x [4 x i32]]* @dct_luma.m4, i64 0, i64 3, i64 1), align 4
  %0 = load i32, i32* getelementptr inbounds ([4 x [4 x i32]], [4 x [4 x i32]]* @dct_luma.m4, i64 0, i64 3, i64 0), align 4
  %sub355 = add nsw i32 undef, %0
  %shr.i = ashr i32 %sub355, 6
  %arrayidx372 = getelementptr inbounds %struct.ImageParameters.11.131.155.323.491.899.923.947.971.995.1043.1067.1091.1115.1139.1187.1235.1307.1331.1355.1379.1595.1691.1883.1907.2027.2099.2387.2411.2507.2531.2771.3179.3203.3227.3251.3275.3443.3467.3683.4115.4379.4859.6058.10.21.32.43.54.549.560, %struct.ImageParameters.11.131.155.323.491.899.923.947.971.995.1043.1067.1091.1115.1139.1187.1235.1307.1331.1355.1379.1595.1691.1883.1907.2027.2099.2387.2411.2507.2531.2771.3179.3203.3227.3251.3275.3443.3467.3683.4115.4379.4859.6058.10.21.32.43.54.549.560* undef, i64 0, i32 52, i64 2, i64 0
  store i32 %shr.i, i32* %arrayidx372, align 4
  %sub355.1 = add nsw i32 undef, %add277
  %shr.i.1 = ashr i32 %sub355.1, 6
  %arrayidx372.1 = getelementptr inbounds %struct.ImageParameters.11.131.155.323.491.899.923.947.971.995.1043.1067.1091.1115.1139.1187.1235.1307.1331.1355.1379.1595.1691.1883.1907.2027.2099.2387.2411.2507.2531.2771.3179.3203.3227.3251.3275.3443.3467.3683.4115.4379.4859.6058.10.21.32.43.54.549.560, %struct.ImageParameters.11.131.155.323.491.899.923.947.971.995.1043.1067.1091.1115.1139.1187.1235.1307.1331.1355.1379.1595.1691.1883.1907.2027.2099.2387.2411.2507.2531.2771.3179.3203.3227.3251.3275.3443.3467.3683.4115.4379.4859.6058.10.21.32.43.54.549.560* undef, i64 0, i32 52, i64 2, i64 1
  store i32 %shr.i.1, i32* %arrayidx372.1, align 4
  %1 = load i32, i32* getelementptr inbounds ([4 x [4 x i32]], [4 x [4 x i32]]* @dct_luma.m4, i64 0, i64 3, i64 2), align 4
  %sub355.2 = add nsw i32 undef, %1
  %shr.i.2 = ashr i32 %sub355.2, 6
  %arrayidx372.2 = getelementptr inbounds %struct.ImageParameters.11.131.155.323.491.899.923.947.971.995.1043.1067.1091.1115.1139.1187.1235.1307.1331.1355.1379.1595.1691.1883.1907.2027.2099.2387.2411.2507.2531.2771.3179.3203.3227.3251.3275.3443.3467.3683.4115.4379.4859.6058.10.21.32.43.54.549.560, %struct.ImageParameters.11.131.155.323.491.899.923.947.971.995.1043.1067.1091.1115.1139.1187.1235.1307.1331.1355.1379.1595.1691.1883.1907.2027.2099.2387.2411.2507.2531.2771.3179.3203.3227.3251.3275.3443.3467.3683.4115.4379.4859.6058.10.21.32.43.54.549.560* undef, i64 0, i32 52, i64 2, i64 2
  store i32 %shr.i.2, i32* %arrayidx372.2, align 4
  %2 = load i32, i32* getelementptr inbounds ([4 x [4 x i32]], [4 x [4 x i32]]* @dct_luma.m4, i64 0, i64 3, i64 3), align 4
  %sub355.3 = add nsw i32 undef, %2
  %shr.i.3 = ashr i32 %sub355.3, 6
  %arrayidx372.3 = getelementptr inbounds %struct.ImageParameters.11.131.155.323.491.899.923.947.971.995.1043.1067.1091.1115.1139.1187.1235.1307.1331.1355.1379.1595.1691.1883.1907.2027.2099.2387.2411.2507.2531.2771.3179.3203.3227.3251.3275.3443.3467.3683.4115.4379.4859.6058.10.21.32.43.54.549.560, %struct.ImageParameters.11.131.155.323.491.899.923.947.971.995.1043.1067.1091.1115.1139.1187.1235.1307.1331.1355.1379.1595.1691.1883.1907.2027.2099.2387.2411.2507.2531.2771.3179.3203.3227.3251.3275.3443.3467.3683.4115.4379.4859.6058.10.21.32.43.54.549.560* undef, i64 0, i32 52, i64 2, i64 3
  store i32 %shr.i.3, i32* %arrayidx372.3, align 4
  unreachable
}
