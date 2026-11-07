<?php

declare(strict_types=1);

require_once __DIR__.'/Fixtures/scenarios.php';

/*
 * Held byte for byte to tests/Fixtures/blend.json, captured by tests/Fixtures/capture.php from
 * the build before the table-driven blend.
 */

it('paints spans in every format as the reference build: random spans, the row edges, 256 coverages in one call, whole rows', function (): void {
    $expected = blendExpected();
    $wrong = [];
    foreach (blendSpanScenarios() as $name => $paint) {
        if (sha1($paint()) !== $expected[$name]) {
            $wrong[] = $name;
        }
    }

    expect($wrong)->toBe([]);
});
