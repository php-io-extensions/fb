<?php

namespace Zephir\Optimizers\FunctionCall;

use Zephir\Call;
use Zephir\CompilationContext;
use Zephir\CompiledExpression;
use Zephir\Exception\CompilerException;
use Zephir\Optimizers\OptimizerAbstract;

class FbBlitRgba8Optimizer extends OptimizerAbstract
{
    public function optimize(array $expression, Call $call, CompilationContext $context)
    {
        if (!isset($expression['parameters']) || count($expression['parameters']) !== 6) {
            throw new CompilerException("'fb_blit_rgba8' requires exactly six parameters", $expression);
        }
        $resolvedParams = $call->getReadOnlyResolvedParams($expression['parameters'], $context, $expression);
        $context->headersManager->add('src/fb-api');
        $context->codePrinter->output('fb_api_blit_rgba8(' . implode(', ', $resolvedParams) . ');');

        return new CompiledExpression('null', null, $expression);
    }
}
