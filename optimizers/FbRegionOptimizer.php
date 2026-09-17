<?php

namespace Zephir\Optimizers\FunctionCall;

use Zephir\Call;
use Zephir\CompilationContext;
use Zephir\CompiledExpression;
use Zephir\Exception\CompilerException;
use Zephir\Optimizers\OptimizerAbstract;

class FbRegionOptimizer extends OptimizerAbstract
{
    public function optimize(array $expression, Call $call, CompilationContext $context)
    {
        if (!isset($expression['parameters']) || count($expression['parameters']) !== 6) {
            throw new CompilerException("'fb_region' requires exactly six parameters", $expression);
        }
        [$symbolVariable, $resolvedParams, $symbol] = $this->processStringOptimizer($call, $context, $expression);
        $context->headersManager->add('src/fb-api');
        $context->codePrinter->output('fb_api_region(' . $symbol . ', ' . implode(', ', $resolvedParams) . ');');

        return new CompiledExpression('variable', $symbolVariable->getRealName(), $expression);
    }
}
