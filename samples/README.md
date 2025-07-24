# Market Index Data (MID) price time series

https://developer.data.elexon.co.uk/api-details#api=prod-insol-insights-api&operation=get-balancing-pricing-market-index-from-from-to-to

This endpoint provides Market Index Data received from NGESO.

Market Index Data is a key component in the calculation of System Buy Price and System Sell Price for each Settlement Period. This data is received from each of the appointed Market Index Data Providers (MIDPs) and reflects the price of wholesale electricity in Great Britain in the short term markets. The Market Index Data which is received from each MIDP for each Settlement Period consists of a Market Index Volume and Market Index Price, representing the volume and price of trading for the relevant period in the market operated by the MIDP. The Market Price (the volume weighed average Market Index Price) is used to derive the Reverse Price (SBP or SSP)."

The two data providers available to query are N2EX ("N2EXMIDP") and APX ("APXMIDP").

By default, the from and to parameters filter the data by time inclusively. If the settlementPeriodFrom or settlementPeriodTo parameters are provided, the corresponding from or to parameter instead filters on settlement date, allowing for searching by a combination of time and/or settlement date & settlement period. Note: When filtering via settlement date, from/to are treated as Dates only, with the time being ignored. For example, 2022-06-01T00:00Z and 2022-06-01T11:11Z are both treated as the settlement date 2022-06-01.

All Dates and DateTimes should be expressed as defined within RFC 3339.

Some examples of date parameter combinations are shown below.

Filtering from start time to start time:
```
/balancing/pricing/market-index?from=2022-06-01T00:00Z&to=2022-07-01T00:00Z
```
Filtering from start time to settlement date and period:
```
/balancing/pricing/market-index?from=2022-06-01T00:00Z&to=2022-07-01T00:00Z&settlementPeriodTo=1
```
Filtering from settlement date and period to start time:
```
/balancing/pricing/market-index?from=2022-06-01T00:00Z&to=2022-07-01T00:00Z&settlementPeriodFrom=1
```
Filtering from settlement date and period to settlement date and period:
```
/balancing/pricing/market-index?from=2022-06-01T00:00Z&to=2022-07-01T00:00Z&settlementPeriodFrom=1&settlementPeriodTo=1
```