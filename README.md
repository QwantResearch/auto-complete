# Auto-complete API

This auto-complete API enables to do suggestions from few letters coming from the queries and do some spell checking on the queries.
Spell checked queries are re-send to the suggestion module to propose new suggestions.

The approaches used to do spell checking is named SymSpell (https://github.com/wolfgarbe/SymSpell). The C++ version was refactored and reimplemented to fit the needs.

The sugestion module uses a segment tree approach (https://en.wikipedia.org/wiki/Segment_tree) which enable to propose solutions the fastest in the State-of-the-Art.

# Installation

```bash ./install.sh```

Or with docker:

```docker build -t "autocompleteimage" .```

```docker run -p 9009:9009 -d --rm autocompleteimage -c models_config.yaml```

# Query the API

```curl -X POST http://localhost:9009/autocomplete/ -H 'Content-Type: application/json' -d '{"text":"jonny","language":"fr","domain","data_fr","count":5}'```

expected result:

```
{
    "corrections": [
        {
            "correction": "jonny",
            "score": 3163.0,
            "suggestions": [
                [
                    3163.0,
                    "jonny"
                ],
                [
                    207.0,
                    "jonny84"
                ],
                [
                    72.0,
                    "jonny-mt"
                ],
                [
                    67.0,
                    "jonny walker"
                ],
                [
                    65.0,
                    "jonny greenwood"
                ]
            ]
        },
        {
            "correction": "johnny",
            "score": 28276.0,
            "suggestions": [
                [
                    28276.0,
                    "johnny"
                ],
                [
                    1647.0,
                    "johnny hallyday"
                ],
                [
                    864.0,
                    "johnny cash"
                ],
                [
                    644.0,
                    "johnny depp"
                ],
                [
                    317.0,
                    "johnny english"
                ]
            ]
        },
        {
            "correction": "jenny",
            "score": 12506.0,
            "suggestions": [
                [
                    12506.0,
                    "jenny"
                ],
                [
                    1980.0,
                    "jennyfer"
                ],
                [
                    320.0,
                    "jennyf"
                ],
                [
                    320.0,
                    "jenny scordamaglia"
                ],
                [
                    307.0,
                    "jenny &"
                ]
            ]
        },
        {
            "correction": "sonny",
            "score": 4189.0,
            "suggestions": [
                [
                    4189.0,
                    "sonny"
                ],
                [
                    231.0,
                    "sonny rollins"
                ],
                [
                    192.0,
                    "sonny boy"
                ],
                [
                    157.0,
                    "sonny liston"
                ],
                [
                    141.0,
                    "sonny stitt"
                ]
            ]
        },
        {
            "correction": "Sonny",
            "score": 3653.0,
            "suggestions": [
                [
                    3653.0,
                    "Sonny"
                ],
                [
                    194.0,
                    "Sonny Rollins"
                ],
                [
                    164.0,
                    "Sonny Boy"
                ],
                [
                    127.0,
                    "Sonny Stitt"
                ],
                [
                    90.0,
                    "Sonny &"
                ]
            ]
        }
    ],
    "count": 5,
    "domain": "fr_data",
    "language": "fr",
    "suggestions": [
        [
            3163.0,
            "jonny"
        ],
        [
            207.0,
            "jonny84"
        ],
        [
            72.0,
            "jonny-mt"
        ],
        [
            67.0,
            "jonny walker"
        ],
        [
            65.0,
            "jonny greenwood"
        ]
    ],
    "text": "jonny"
}
```







