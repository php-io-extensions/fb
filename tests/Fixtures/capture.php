<?php

declare(strict_types=1);

// Records what a build paints for every blend scenario: run it with the build to hold later ones to.
//   php -d extension=<build>/modules/fb.so tests/Fixtures/capture.php [out.json]
require __DIR__.'/scenarios.php';

$hashes = [];
foreach ([...blendTableScenarios(), ...blendSpanScenarios(), ...blendImageScenarios()] as $name => $paint) {
    $hashes[$name] = sha1($paint());
}
file_put_contents($argv[1] ?? __DIR__.'/blend.json', json_encode($hashes, JSON_PRETTY_PRINT | JSON_UNESCAPED_SLASHES)."\n");
printf("%d scenarios\n", count($hashes));
