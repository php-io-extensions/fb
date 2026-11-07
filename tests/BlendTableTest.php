<?php

declare(strict_types=1);

require_once __DIR__.'/Fixtures/scenarios.php';

/*
 * Held byte for byte to tests/Fixtures/blend.json, captured by tests/Fixtures/capture.php from
 * the build before the table-driven blend.
 */

it('paints a span over every destination level as the reference build, for every colour level and alpha', function (): void {
    $expected = blendExpected();
    $wrong = [];
    foreach (blendTableScenarios() as $name => $paint) {
        if (sha1($paint()) !== $expected[$name]) {
            $wrong[] = $name;
        }
    }

    expect($wrong)->toBe([]);
});
