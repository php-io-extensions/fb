<?php

declare(strict_types=1);

require_once __DIR__.'/Fixtures/scenarios.php';

/*
 * Held byte for byte to tests/Fixtures/blend.json, captured by tests/Fixtures/capture.php from
 * the build before the table-driven blend.
 */

it('paints an RGBA8 image as the reference build: identity, scale, turn and mirror, nearest and smooth, two opacities', function (): void {
    $expected = blendExpected();
    $wrong = [];
    foreach (blendImageScenarios() as $name => $paint) {
        if (sha1($paint()) !== $expected[$name]) {
            $wrong[] = $name;
        }
    }

    expect($wrong)->toBe([]);
});
