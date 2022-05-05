(
  to_entries[] | .key as $key | .value |
    ("\($key),name \(.name)"),
    ("\($key),region \(.state)"),
    ("\($key),country \(.country)"),
    ("\($key),latitude \(.lat)"),
    ("\($key),longitude \(.lon)")
),

("count \(. | length)")
