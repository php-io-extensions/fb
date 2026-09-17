<?php

namespace Zephir\Optimizers\FunctionCall;

use Zephir\Call;
use Zephir\CompilationContext;
use Zephir\CompiledExpression;
use Zephir\Exception\CompilerException;
use Zephir\Optimizers\OptimizerAbstract;

class FbTranscodeOptimizer extends OptimizerAbstract
{
    public function optimize(array $expression, Call $call, CompilationContext $context)
    {
        if (!isset($expression['parameters']) || count($expression['parameters']) !== 12) {
            throw new CompilerException("'fb_transcode' requires exactly twelve parameters", $expression);
        }
        [$symbolVariable, $resolvedParams, $symbol] = $this->processStringOptimizer($call, $context, $expression);
        $context->headersManager->add('src/fb-api');
        $context->codePrinter->output('fb_api_transcode(' . $symbol . ', ' . implode(', ', $resolvedParams) . ');');

        return new CompiledExpression('variable', $symbolVariable->getRealName(), $expression);
    }
}
