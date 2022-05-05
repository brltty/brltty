(
  .city |
    ("location,name \(.name)"),
    ("location,country \(.country)"),
    ("sun,rise \(.sunrise)"),
    ("sun,set \(.sunset)"),
    ("time,offset \(.timezone)")
),

(
  .list |
    (
      to_entries[] | .key as $key | .value |
        (
          .weather[0] |
            ("\($key),weather,general \(.main)"),
            ("\($key),weather,description \(.description)")
        ),

        (
          .main |
            ("\($key),temperature \(.temp)"),
            ("\($key),temperature,feels \(.feels_like)"),
            ("\($key),temperature,minimum \(.temp_min)"),
            ("\($key),temperature,maximum \(.temp_max)"),
            ("\($key),pressure \(.pressure)"),
            ("\($key),pressure,sea \(.sea_level)"),
            ("\($key),pressure,ground \(.grnd_level)"),
            ("\($key),humidity \(.humidity)")
        ),

        (
          .wind |
            ("\($key),wind,speed \(.speed)"),
            ("\($key),wind,direction \(.deg)"),
            ("\($key),wind,gust \(.gust)")
        ),

        (
          .clouds |
            ("\($key),clouds,percent \(.all)")
        ),

        ("\($key),pop \(.pop)"),
        ("\($key),humidity \(.humidity)"),
        ("\($key),time \(.dt)")
    ),

    ("count \(length)")
)
